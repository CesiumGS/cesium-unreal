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

namespace {
int32 getLineCount(int32 numPoints, bool isPolyline) {
  return isPolyline ? numPoints - 1 : numPoints / 2;
}
} // namespace

FCesiumGltfLinesSceneProxy::FCesiumGltfLinesSceneProxy(
    UCesiumGltfLinesComponent* InComponent,
    ERHIFeatureLevel::Type InFeatureLevel)
    : FPrimitiveSceneProxy(InComponent),
      RenderData(InComponent->GetStaticMesh()->GetRenderData()),
      NumLines(getLineCount(
          RenderData->LODResources[0].IndexBuffer.GetNumIndices(),
          InComponent->IsPolyline)),
      IsPolyline(InComponent->IsPolyline),
      LineWidth(InComponent->LineWidth),
      PolylineVertexFactory(
          InFeatureLevel,
          &RenderData->LODResources[0].VertexBuffers.PositionVertexBuffer),
      PolylineIndexBuffer(NumLines, true),
      Material(InComponent->GetMaterial(0)),
      MaterialRelevance(InComponent->GetMaterialRelevance(InFeatureLevel)),
      bManualVertexFetchSupported(
          RHISupportsManualVertexFetch(GetScene().GetShaderPlatform())) {}

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
  if (!HasViewDependentDPG() && !IsPolyline) {
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
      if (IsPolyline) {
        CreatePolylineMesh(Mesh, View, Collector);
      } else {
        CreateMesh(Mesh);
      }
      Collector.AddMesh(ViewIndex, Mesh);
    }
  }
}

FPrimitiveViewRelevance
FCesiumGltfLinesSceneProxy::GetViewRelevance(const FSceneView* View) const {
  FPrimitiveViewRelevance Result;
  Result.bDrawRelevance = IsShown(View);

  if (IsPolyline) {
    // TODO can you do polyline rendering with static relevance?
    Result.bStaticRelevance = false;
    Result.bDynamicRelevance = true;
  } else {
    if (HasViewDependentDPG()) {
      Result.bDynamicRelevance = true;
    } else {
      Result.bStaticRelevance = true;
    }
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

void FCesiumGltfLinesSceneProxy::CreatePolylineUserData(
    FMeshBatchElement& BatchElement,
    const FSceneView* View,
    FMeshElementCollector& Collector) const {
  FCesiumPolylineBatchElementUserDataWrapper* pUserDataWrapper =
      &Collector.AllocateOneFrameResource<
          FCesiumPolylineBatchElementUserDataWrapper>();

  FCesiumPolylineBatchElementUserData& UserData = pUserDataWrapper->Data;
  const FLocalVertexFactory& OriginalVertexFactory =
      RenderData->LODVertexFactories[0].VertexFactory;

  UserData.PositionBuffer = OriginalVertexFactory.GetPositionsSRV();
  UserData.PackedTangentsBuffer = OriginalVertexFactory.GetTangentsSRV();
  UserData.ColorBuffer = OriginalVertexFactory.GetColorComponentsSRV();
  UserData.TexCoordBuffer = OriginalVertexFactory.GetTextureCoordinatesSRV();
  UserData.NumTexCoords = OriginalVertexFactory.GetNumTexcoords();

  UserData.NumPolylinePoints = this->NumLines + 1;
  UserData.LineWidth = LineWidth;

  BatchElement.UserData = &pUserDataWrapper->Data;
}

void FCesiumGltfLinesSceneProxy::CreatePolylineMesh(
    FMeshBatch& Mesh,
    const FSceneView* View,
    FMeshElementCollector& Collector) const {
  Mesh.VertexFactory = &PolylineVertexFactory;
  Mesh.MaterialRenderProxy = Material->GetRenderProxy();
  Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
  Mesh.Type = PT_TriangleList;
  Mesh.DepthPriorityGroup = SDPG_World;
  Mesh.LODIndex = 0;
  Mesh.bCanApplyViewModeOverrides = false;
  Mesh.bUseAsOccluder = false;
  Mesh.bWireframe = false;

  FMeshBatchElement& BatchElement = Mesh.Elements[0];
  BatchElement.IndexBuffer = &PolylineIndexBuffer;
  BatchElement.NumPrimitives = NumLines * 2; // Two triangles per line.
  BatchElement.FirstIndex = 0;
  BatchElement.MinVertexIndex = 0;
  BatchElement.MaxVertexIndex = NumLines * 4 - 1;
  BatchElement.PrimitiveUniformBuffer = GetUniformBuffer();

  CreatePolylineUserData(BatchElement, View, Collector);
}

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
