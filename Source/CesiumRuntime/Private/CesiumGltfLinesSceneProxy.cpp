// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumGltfLinesSceneProxy.h"
#include "CesiumGltfLinesComponent.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "Engine/StaticMesh.h"
#include "RHIResources.h"
#include "Runtime/Launch/Resources/Version.h"
#include "SceneInterface.h"
#include "SceneView.h"
#include "StaticMeshResources.h"

SIZE_T FCesiumGltfLinesSceneProxy::GetTypeHash() const {
  static size_t UniquePointer;
  return reinterpret_cast<size_t>(&UniquePointer);
}

FCesiumGltfLinesSceneProxy::FCesiumGltfLinesSceneProxy(
    UCesiumGltfLinesComponent* InComponent,
    ERHIFeatureLevel::Type InFeatureLevel)
    : FPrimitiveSceneProxy(InComponent),
      RenderData(InComponent->GetStaticMesh()->GetRenderData()),
      NumLines(RenderData->LODResources[0].IndexBuffer.GetNumIndices() / 2),
      PolylineVertexFactory(
          InFeatureLevel,
          &RenderData->LODResources[0].VertexBuffers.PositionVertexBuffer),
      PolylineIndexBuffer(NumLines, true),
    //  LineMode(InComponent->getPrimitiveData().pMeshPrimitive->mode),
      Material(InComponent->GetMaterial(0)),
      MaterialRelevance(InComponent->GetMaterialRelevance(InFeatureLevel)) {}

FCesiumGltfLinesSceneProxy::~FCesiumGltfLinesSceneProxy() {}

void FCesiumGltfLinesSceneProxy::CreateRenderThreadResources(
    FRHICommandListBase& RHICmdList) {
  PolylineVertexFactory.InitResource(RHICmdList);
  PolylineIndexBuffer.InitResource(RHICmdList);
}

void FCesiumGltfLinesSceneProxy::DestroyRenderThreadResources() {
  PolylineVertexFactory.ReleaseResource();
  PolylineIndexBuffer.ReleaseResource(); 
}

void FCesiumGltfLinesSceneProxy::DrawStaticElements(
    FStaticPrimitiveDrawInterface* PDI) {
  if (!HasViewDependentDPG()) {
    FMeshBatch Mesh;
    CreateMesh(Mesh);
    PDI->DrawMesh(Mesh, FLT_MAX);
  }
}

void FCesiumGltfLinesSceneProxy::GetDynamicMeshElements(
    const TArray<const FSceneView*>& Views,
    const FSceneViewFamily& ViewFamily,
    uint32 VisibilityMap,
    FMeshElementCollector& Collector) const {
  QUICK_SCOPE_CYCLE_COUNTER(STAT_GltfLinesSceneProxy_GetDynamicMeshElements);

  for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++) {
    if (VisibilityMap & (1 << ViewIndex)) {
      const FSceneView* View = Views[ViewIndex];
      FMeshBatch& Mesh = Collector.AllocateMesh();
      CreateMesh(Mesh);
      Collector.AddMesh(ViewIndex, Mesh);
    }
  }
}

FPrimitiveViewRelevance
FCesiumGltfLinesSceneProxy::GetViewRelevance(const FSceneView* View) const {
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

uint32 FCesiumGltfLinesSceneProxy::GetMemoryFootprint(void) const {
  return (sizeof(*this) + GetAllocatedSize());
}

// void FCesiumGltfLinesSceneProxy::CreatePointAttenuationUserData(
//    FMeshBatchElement& BatchElement,
//    const FSceneView* View,
//    FMeshElementCollector& Collector) const {
//  FCesiumPointAttenuationBatchElementUserDataWrapper* UserDataWrapper =
//      &Collector.AllocateOneFrameResource<
//          FCesiumPointAttenuationBatchElementUserDataWrapper>();
//
//  FCesiumPointAttenuationBatchElementUserData& UserData = UserDataWrapper->Data;
//  const FLocalVertexFactory& OriginalVertexFactory =
//      RenderData->LODVertexFactories[0].VertexFactory;
//
//  UserData.PositionBuffer = OriginalVertexFactory.GetPositionsSRV();
//  UserData.PackedTangentsBuffer = OriginalVertexFactory.GetTangentsSRV();
//  UserData.ColorBuffer = OriginalVertexFactory.GetColorComponentsSRV();
//  UserData.TexCoordBuffer = OriginalVertexFactory.GetTextureCoordinatesSRV();
//  UserData.NumTexCoords = OriginalVertexFactory.GetNumTexcoords();
//  UserData.bHasPointColors = RenderData->LODResources[0].bHasColorVertexData;
//
//  FCesiumPointCloudShading PointCloudShading = TilesetData.PointCloudShading;
//
//  float MaximumLinesize = TilesetData.UsesAdditiveRefinement
//                               ? 5.0f
//                               : TilesetData.MaximumScreenSpaceError;
//
//  if (PointCloudShading.MaximumAttenuation > 0.0f) {
//    // Don't multiply by DPI scale; let Unreal handle scaling.
//    MaximumLinesize = PointCloudShading.MaximumAttenuation;
//  }
//
//  float GeometricError = GetGeometricError();
//  GeometricError *= PointCloudShading.GeometricErrorScale;
//
//  // Depth Multiplier
//  float SSEDenominator = 2.0f * tanf(0.5f *
//  FMath::DegreesToRadians(View->FOV)); float DepthMultiplier =
//      static_cast<float>(View->UnconstrainedViewRect.Height()) /
//      SSEDenominator;
//
//  UserData.AttenuationParameters =
//      FVector3f(MaximumLinesize, GeometricError, DepthMultiplier);
//
//  BatchElement.UserData = &UserDataWrapper->Data;
//}
//
//void FCesiumGltfLinesSceneProxy::CreateMeshWithAttenuation(
//    FMeshBatch& Mesh,
//    const FSceneView* View,
//    FMeshElementCollector& Collector) const {
//  Mesh.VertexFactory = &AttenuationVertexFactory;
//  Mesh.MaterialRenderProxy = Material->GetRenderProxy();
//  Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
//  Mesh.Type = PT_TriangleList;
//  Mesh.DepthPriorityGroup = SDPG_World;
//  Mesh.LODIndex = 0;
//  Mesh.bCanApplyViewModeOverrides = false;
//  Mesh.bUseAsOccluder = false;
//  Mesh.bWireframe = false;
//
//  FMeshBatchElement& BatchElement = Mesh.Elements[0];
//  BatchElement.IndexBuffer = &AttenuationIndexBuffer;
//  BatchElement.NumPrimitives = NumLines * 2;
//  BatchElement.FirstIndex = 0;
//  BatchElement.MinVertexIndex = 0;
//  BatchElement.MaxVertexIndex = NumLines * 4 - 1;
//  BatchElement.PrimitiveUniformBuffer = GetUniformBuffer();
//
//  CreatePointAttenuationUserData(BatchElement, View, Collector);
//}

void FCesiumGltfLinesSceneProxy::CreateMesh(FMeshBatch& Mesh) const {
  Mesh.VertexFactory = &RenderData->LODVertexFactories[0].VertexFactory;
  Mesh.MaterialRenderProxy = Material->GetRenderProxy();
  Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
  Mesh.Type = PT_LineList;
  Mesh.DepthPriorityGroup = SDPG_World;
  Mesh.LODIndex = 0;
  Mesh.bCanApplyViewModeOverrides = false;
  Mesh.bUseAsOccluder = false;
  Mesh.bWireframe = false;

  FMeshBatchElement& BatchElement = Mesh.Elements[0];
  BatchElement.IndexBuffer = &RenderData->LODResources[0].IndexBuffer;
  BatchElement.NumPrimitives = NumLines;
  BatchElement.FirstIndex = 0;
  BatchElement.MinVertexIndex = 0;
  BatchElement.MaxVertexIndex = BatchElement.NumPrimitives - 1;
}
