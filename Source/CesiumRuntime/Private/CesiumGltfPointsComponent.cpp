// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumGltfPointsComponent.h"

#include "Cesium3DTileset.h"
#include "CesiumGltfPointsVertexFactory.h"
#include "Engine/StaticMesh.h"

class FCesiumGltfPointsSceneProxy final : public FPrimitiveSceneProxy {
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
        VertexFactory(
            InFeatureLevel,
            RenderData->LODResources[0].VertexBuffers),
        IndexBuffer(),
        Material(InComponent->GetMaterial(0)),
        MaterialRelevance(
            InComponent->GetMaterialRelevance(GetScene().GetFeatureLevel())) {}

  virtual ~FCesiumGltfPointsSceneProxy() {}

  virtual void GetDynamicMeshElements(
      const TArray<const FSceneView*>& Views,
      const FSceneViewFamily& ViewFamily,
      uint32 VisibilityMap,
      FMeshElementCollector& Collector) const override {
    QUICK_SCOPE_CYCLE_COUNTER(STAT_GltfPointsSceneProxy_GetDynamicMeshElements);

    const bool hasAttenuation =
        this->_pTileset->GetPointCloudShading().Attenuation;

    for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++) {
      if (VisibilityMap & (1 << ViewIndex)) {
        const FSceneView* View = Views[ViewIndex];
        FMeshBatch& Mesh = Collector.AllocateMesh();
        if (hasAttenuation) {
          CreateMeshWithAttenuation(Mesh);
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

    Result.bRenderCustomDepth = ShouldRenderCustomDepth();
    Result.bRenderInMainPass = ShouldRenderInMainPass();
    Result.bRenderInDepthPass = ShouldRenderInDepthPass();
    Result.bUsesLightingChannels =
        GetLightingChannelMask() != GetDefaultLightingChannelMask();
    Result.bShadowRelevance = IsShadowCast(View);
    Result.bVelocityRelevance =
        IsMovable() & Result.bOpaque & Result.bRenderInMainPass;

    MaterialRelevance.SetPrimitiveViewRelevance(Result);

    return Result;
  }

  virtual uint32 GetMemoryFootprint(void) const override {
    return (sizeof(*this) + GetAllocatedSize());
  }

private:
  // The original render data of the static mesh.
  const FStaticMeshRenderData* RenderData;

  // The vertex factory and index buffer for point attenuation.
  FCesiumGltfPointsVertexFactory VertexFactory;
  FIndexBuffer IndexBuffer;

  UMaterialInterface* Material;
  FMaterialRelevance MaterialRelevance;

  int32_t _numPoints;
  ACesium3DTileset* _pTileset;

  void CreateMeshWithAttenuation(FMeshBatch& Mesh) const {
    Mesh.VertexFactory = &VertexFactory;
    Mesh.MaterialRenderProxy = Material->GetRenderProxy();
    Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
    Mesh.Type = PT_TriangleList;
    Mesh.DepthPriorityGroup = SDPG_World;
    Mesh.LODIndex = 0;
    Mesh.bCanApplyViewModeOverrides = false;
    Mesh.bUseAsOccluder = false;
    Mesh.bWireframe = false;

    auto pIndexBuffer = &IndexBuffer;
    FMeshBatchElement& BatchElement = Mesh.Elements[0];
    BatchElement.IndexBuffer = pIndexBuffer;
    BatchElement.NumPrimitives = _numPoints * 2;
    BatchElement.FirstIndex = 0;
    BatchElement.MinVertexIndex = 0;
    BatchElement.MaxVertexIndex = _numPoints;
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

    auto pIndexBuffer = &RenderData->LODResources[0].IndexBuffer;
    FMeshBatchElement& BatchElement = Mesh.Elements[0];
    BatchElement.IndexBuffer = pIndexBuffer;
    BatchElement.NumPrimitives = pIndexBuffer->GetNumIndices();
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
