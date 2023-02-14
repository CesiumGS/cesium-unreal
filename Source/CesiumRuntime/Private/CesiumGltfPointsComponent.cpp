// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumGltfPointsComponent.h"
#include "Engine/StaticMesh.h"

class FGltfPointsSceneProxy final : public FPrimitiveSceneProxy {
public:
  SIZE_T GetTypeHash() const override {
    static size_t UniquePointer;
    return reinterpret_cast<size_t>(&UniquePointer);
  }

  FGltfPointsSceneProxy(
      UCesiumGltfPointsComponent* InComponent,
      ERHIFeatureLevel::Type InFeatureLevel)
      : FPrimitiveSceneProxy(InComponent) {
    const auto RenderData = InComponent->GetStaticMesh()->GetRenderData();
    LODResources = &RenderData->LODResources[0];
    VertexFactory = &RenderData->LODVertexFactories[0].VertexFactory;
    Material = InComponent->GetMaterial(0);
    MaterialRelevance =
        InComponent->GetMaterialRelevance(GetScene().GetFeatureLevel());
  }

  virtual ~FGltfPointsSceneProxy() {}

  virtual void DrawStaticElements(FStaticPrimitiveDrawInterface* PDI) override {
    if (!HasViewDependentDPG()) {
      FMeshBatch Mesh;
      CreateMesh(Mesh);
      PDI->DrawMesh(Mesh, FLT_MAX);
    }
  }

  virtual void GetDynamicMeshElements(
      const TArray<const FSceneView*>& Views,
      const FSceneViewFamily& ViewFamily,
      uint32 VisibilityMap,
      FMeshElementCollector& Collector) const override {
    QUICK_SCOPE_CYCLE_COUNTER(STAT_GltfPointsSceneProxy_GetDynamicMeshElements);

    for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++) {
      if (VisibilityMap & (1 << ViewIndex)) {
        const FSceneView* View = Views[ViewIndex];
        FMeshBatch& Mesh = Collector.AllocateMesh();
        CreateMesh(Mesh);
        Collector.AddMesh(ViewIndex, Mesh);
      }
    }
  }

  virtual FPrimitiveViewRelevance
  GetViewRelevance(const FSceneView* View) const override {
    FPrimitiveViewRelevance Result;
    Result.bDrawRelevance = IsShown(View);
    if (HasViewDependentDPG()) {
      Result.bDynamicRelevance = true;
    } else {
      Result.bStaticRelevance = true;
    }

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
  FLocalVertexFactory* VertexFactory;
  FStaticMeshLODResources* LODResources;
  UMaterialInterface* Material;
  FMaterialRelevance MaterialRelevance;

  void CreateMesh(FMeshBatch& Mesh) const {
    Mesh.VertexFactory = VertexFactory;
    Mesh.MaterialRenderProxy = Material->GetRenderProxy();
    Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
    Mesh.Type = PT_PointList;
    Mesh.DepthPriorityGroup = SDPG_World;
    Mesh.LODIndex = 0;
    Mesh.bCanApplyViewModeOverrides = false;
    Mesh.bUseAsOccluder = false;
    Mesh.bWireframe = false;

    auto pIndexBuffer = &LODResources->IndexBuffer;
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
  return new FGltfPointsSceneProxy(this, GetScene()->GetFeatureLevel());
}
