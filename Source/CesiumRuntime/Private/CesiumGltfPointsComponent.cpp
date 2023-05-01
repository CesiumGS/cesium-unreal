// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumGltfPointsComponent.h"
#include "Cesium3DTileset.h"
#include "CesiumPointAttenuationVertexFactory.h"
#include "Engine/StaticMesh.h"
#include "Engine/UserInterfaceSettings.h"

class FCesiumGltfPointsSceneProxy final : public FPrimitiveSceneProxy {
private:
  // The original render data of the static mesh.
  const FStaticMeshRenderData* RenderData;
  int32_t NumPoints;

public:
  SIZE_T GetTypeHash() const override {
    static size_t UniquePointer;
    return reinterpret_cast<size_t>(&UniquePointer);
  }

  FCesiumGltfPointsSceneProxy(
      UCesiumGltfPointsComponent* InComponent,
      ERHIFeatureLevel::Type InFeatureLevel)
      : FPrimitiveSceneProxy(InComponent),
        RenderData(InComponent->GetStaticMesh()->GetRenderData()),
        NumPoints(RenderData->LODResources[0].IndexBuffer.GetNumIndices()),
        AttenuationVertexFactory(InFeatureLevel),
        AttenuationIndexBuffer(NumPoints),
        Material(InComponent->GetMaterial(0)),
        MaterialRelevance(
            InComponent->GetMaterialRelevance(InFeatureLevel)),
        pGltfPointsComponent(InComponent),
        pTilesetActor(InComponent->pTilesetActor) {}

  virtual ~FCesiumGltfPointsSceneProxy() {}

protected:
  virtual void CreateRenderThreadResources() override {
    AttenuationVertexFactory.InitResource();
    AttenuationIndexBuffer.InitResource();
  }

  virtual void DestroyRenderThreadResources() override {
    AttenuationVertexFactory.ReleaseResource();
    AttenuationIndexBuffer.ReleaseResource();
  }

  virtual void GetDynamicMeshElements(
      const TArray<const FSceneView*>& Views,
      const FSceneViewFamily& ViewFamily,
      uint32 VisibilityMap,
      FMeshElementCollector& Collector) const override {
    QUICK_SCOPE_CYCLE_COUNTER(STAT_GltfPointsSceneProxy_GetDynamicMeshElements);

    if (!pTilesetActor) {
      return;
    }

    const bool hasAttenuation =
        pTilesetActor->GetPointCloudShading().Attenuation;

    for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++) {
      if (VisibilityMap & (1 << ViewIndex)) {
        const FSceneView* View = Views[ViewIndex];
        FMeshBatch& Mesh = Collector.AllocateMesh();

        if (hasAttenuation) {
          CreateMeshWithAttenuation(Mesh, View, Collector);
        } else {
          CreateMesh(Mesh);
        }

        Collector.AddMesh(ViewIndex, Mesh);
      }
    }
  }

  virtual FPrimitiveViewRelevance
  GetViewRelevance(const FSceneView* View) const override {
    FPrimitiveViewRelevance Result;
    Result.bDrawRelevance = IsShown(View);
    // Always render dynamically; the appearance of the points can change
    // via point cloud shading.
    Result.bDynamicRelevance = true;
    Result.bStaticRelevance = false;

    Result.bRenderCustomDepth = ShouldRenderCustomDepth();
    Result.bRenderInMainPass = ShouldRenderInMainPass();
    Result.bRenderInDepthPass = ShouldRenderInDepthPass();
    Result.bUsesLightingChannels =
        GetLightingChannelMask() != GetDefaultLightingChannelMask();
    Result.bShadowRelevance = IsShadowCast(View);
    Result.bVelocityRelevance = false;

    MaterialRelevance.SetPrimitiveViewRelevance(Result);

    return Result;
  }

  virtual uint32 GetMemoryFootprint(void) const override {
    return (sizeof(*this) + GetAllocatedSize());
  }

private:
  // The vertex factory and index buffer for point attenuation.
  FCesiumPointAttenuationVertexFactory AttenuationVertexFactory;
  FCesiumPointAttenuationIndexBuffer AttenuationIndexBuffer;

  UMaterialInterface* Material;
  FMaterialRelevance MaterialRelevance;

  const UCesiumGltfPointsComponent* pGltfPointsComponent;
  const ACesium3DTileset* pTilesetActor;

  void CreatePointAttenuationUserData(
      FMeshBatchElement& BatchElement,
      const FSceneView* View,
      FMeshElementCollector& Collector) const {
    FCesiumPointAttenuationBatchElementUserDataWrapper* UserDataWrapper =
        &Collector.AllocateOneFrameResource<
            FCesiumPointAttenuationBatchElementUserDataWrapper>();

    FCesiumPointAttenuationBatchElementUserData& UserData =
        UserDataWrapper->Data;

    const FLocalVertexFactory& OriginalVertexFactory =
        RenderData->LODVertexFactories[0].VertexFactory;
    UserData.PositionBuffer = OriginalVertexFactory.GetPositionsSRV();
    UserData.PackedTangentsBuffer = OriginalVertexFactory.GetTangentsSRV();
    UserData.ColorBuffer = OriginalVertexFactory.GetColorComponentsSRV();
    UserData.TexCoordBuffer = OriginalVertexFactory.GetTextureCoordinatesSRV();
    UserData.NumTexCoords = OriginalVertexFactory.GetNumTexcoords();
    UserData.bHasPointColors = RenderData->LODResources[0].bHasColorVertexData;

    FCesiumPointCloudShading PointCloudShading =
        pTilesetActor->GetPointCloudShading();

    float MaximumPointSize = pGltfPointsComponent->UsesAdditiveRefinement
                                 ? 5.0f
                                 : pTilesetActor->MaximumScreenSpaceError;

    if (PointCloudShading.MaximumAttenuation > 0.0f) {
      // Don't multiply by DPI; let Unreal handle scaling.
      MaximumPointSize = PointCloudShading.MaximumAttenuation;
    }
    
    float GeometricError = pGltfPointsComponent->GeometricError *
                           PointCloudShading.GeometricErrorScale;

    // Depth Multiplier
    float SSEDenominator =
        2.0f * tanf(0.5f * FMath::DegreesToRadians(View->FOV));
    float DepthMultiplier =
        static_cast<float>(View->UnconstrainedViewRect.Height()) /
        SSEDenominator;

    UserData.AttenuationParameters =
        FVector3f(MaximumPointSize, GeometricError, DepthMultiplier);

    BatchElement.UserData = &UserDataWrapper->Data;
  }

  void CreateMeshWithAttenuation(
      FMeshBatch& Mesh,
      const FSceneView* View,
      FMeshElementCollector& Collector) const {
    Mesh.VertexFactory = &AttenuationVertexFactory;
    Mesh.MaterialRenderProxy = Material->GetRenderProxy();
    Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
    Mesh.Type = PT_TriangleList;
    Mesh.DepthPriorityGroup = SDPG_World;
    Mesh.LODIndex = 0;
    Mesh.bCanApplyViewModeOverrides = false;
    Mesh.bUseAsOccluder = false;
    Mesh.bWireframe = false;

    FMeshBatchElement& BatchElement = Mesh.Elements[0];
    BatchElement.IndexBuffer = &AttenuationIndexBuffer;
    BatchElement.NumPrimitives = NumPoints * 2;
    BatchElement.FirstIndex = 0;
    BatchElement.MinVertexIndex = 0;
    BatchElement.MaxVertexIndex = 0;
    BatchElement.PrimitiveUniformBuffer = GetUniformBuffer();

    CreatePointAttenuationUserData(BatchElement, View, Collector);
  }

  void CreateMesh(FMeshBatch& Mesh) const {
    Mesh.VertexFactory = &RenderData->LODVertexFactories[0].VertexFactory;
    Mesh.MaterialRenderProxy = Material->GetRenderProxy();
    Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
    Mesh.Type = PT_PointList;
    Mesh.DepthPriorityGroup = SDPG_World;
    Mesh.LODIndex = 0;
    Mesh.bCanApplyViewModeOverrides = false;
    Mesh.bUseAsOccluder = false;
    Mesh.bWireframe = false;

    FMeshBatchElement& BatchElement = Mesh.Elements[0];
    BatchElement.IndexBuffer = &RenderData->LODResources[0].IndexBuffer;
    BatchElement.NumPrimitives = NumPoints;
    BatchElement.FirstIndex = 0;
    BatchElement.MinVertexIndex = 0;
    BatchElement.MaxVertexIndex = BatchElement.NumPrimitives - 1;
  }
};

// Sets default values for this component's properties
UCesiumGltfPointsComponent::UCesiumGltfPointsComponent() {}

UCesiumGltfPointsComponent::~UCesiumGltfPointsComponent() {}

FPrimitiveSceneProxy* UCesiumGltfPointsComponent::CreateSceneProxy() {
  return new FCesiumGltfPointsSceneProxy(this, GetScene()->GetFeatureLevel());
}
