// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumGltfPointsComponent.h"
#include "CalcBounds.h"
#include "CesiumLifetime.h"
#include "CesiumMaterialUserData.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PhysicsEngine/BodySetup.h"
#include "VecMath.h"
#include <variant>

class FGltfPointsSceneProxy final : public FPrimitiveSceneProxy {
public:
  SIZE_T GetTypeHash() const override {
    static size_t UniquePointer;
    return reinterpret_cast<size_t>(&UniquePointer);
  }

  FGltfPointsSceneProxy(
      UCesiumGltfPointsComponent* Component,
      ERHIFeatureLevel::Type InFeatureLevel)
      : FPrimitiveSceneProxy(Component),
        MaterialRelevance(
            Component->GetMaterialRelevance(GetScene().GetFeatureLevel())) {
    auto& RenderData = Component->GetStaticMesh()->RenderData;
    LODResources = &RenderData->LODResources[0];
    VertexFactory = &RenderData->LODVertexFactories[0].VertexFactory;
    Material =
        UMaterial::GetDefaultMaterial(MD_Surface); // Component->GetMaterial(0);
  }

  virtual ~FGltfPointsSceneProxy() {}

  virtual void DrawStaticElements(FStaticPrimitiveDrawInterface* PDI) override {
    FMeshBatch Mesh;
    FMeshBatchElement& BatchElement = Mesh.Elements[0];
    Mesh.bWireframe = false;
    Mesh.VertexFactory = VertexFactory;
    Mesh.MaterialRenderProxy = Material->GetRenderProxy();

    bool receivesDecals = true;
    bool drawsVelocity = false;
    BatchElement.IndexBuffer = &(LODResources->IndexBuffer);
    BatchElement.NumPrimitives = LODResources->IndexBuffer.GetNumIndices();
    BatchElement.FirstIndex = 0;
    BatchElement.MinVertexIndex = 0;
    Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
    Mesh.Type = PT_PointList;
    Mesh.DepthPriorityGroup = SDPG_World;
    Mesh.bCanApplyViewModeOverrides = false;

    PDI->DrawMesh(Mesh, FLT_MAX);
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
        // Draw the mesh.
        FMeshBatch& Mesh = Collector.AllocateMesh();
        FMeshBatchElement& BatchElement = Mesh.Elements[0];
        BatchElement.IndexBuffer = &(LODResources->IndexBuffer);
        BatchElement.NumPrimitives = LODResources->IndexBuffer.GetNumIndices();
        BatchElement.FirstIndex = 0;
        BatchElement.MinVertexIndex = 0;

        Mesh.bWireframe = false;
        Mesh.VertexFactory = VertexFactory;
        Mesh.MaterialRenderProxy = Material->GetRenderProxy();
        Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
        Mesh.Type = PT_PointList;
        Mesh.DepthPriorityGroup = SDPG_World;
        Mesh.bCanApplyViewModeOverrides = false;

        Collector.AddMesh(ViewIndex, Mesh);
      }
    }
  }

  virtual FPrimitiveViewRelevance
  GetViewRelevance(const FSceneView* View) const override {
    FPrimitiveViewRelevance Result;
    Result.bDrawRelevance = IsShown(View);
    Result.bShadowRelevance = IsShadowCast(View);
    Result.bDynamicRelevance = true;
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
};

// Sets default values for this component's properties
UCesiumGltfPointsComponent::UCesiumGltfPointsComponent() {}

UCesiumGltfPointsComponent::~UCesiumGltfPointsComponent() {}

FPrimitiveSceneProxy* UCesiumGltfPointsComponent::CreateSceneProxy() {
  return new FGltfPointsSceneProxy(this, GetScene()->GetFeatureLevel());
}
