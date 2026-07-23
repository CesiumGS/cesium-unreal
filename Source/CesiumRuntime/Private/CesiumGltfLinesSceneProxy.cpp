// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumGltfLinesSceneProxy.h"
#include "CesiumGltfLinesComponent.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "Engine/StaticMesh.h"
#include "RHIResources.h"
#include "Runtime/Launch/Resources/Version.h"
#include "SceneInterface.h"
#include "SceneView.h"

SIZE_T FCesiumGltfLinesSceneProxy::GetTypeHash() const {
  static size_t UniquePointer;
  return reinterpret_cast<size_t>(&UniquePointer);
}

FCesiumGltfLinesSceneProxy::FCesiumGltfLinesSceneProxy(
    UCesiumGltfLinesComponent* InComponent,
    FSceneInterfaceWrapper InSceneInterfaceParams)
    : FPrimitiveSceneProxy(InComponent),
      _pRenderData(InComponent->GetStaticMesh()->GetRenderData()),
      _numLines(
          this->_pRenderData->LODResources[0].IndexBuffer.GetNumIndices() / 2),
      _manualVertexFetchSupported(
          RHISupportsManualVertexFetch(GetScene().GetShaderPlatform())),
      _lineWidth(InComponent->width),
      _pattern(InComponent->pattern),
      _indexBufferSRV(),
      _lineStyleVertexFactory(
          InSceneInterfaceParams.RHIFeatureLevelType,
          &this->_pRenderData->LODResources[0]
               .VertexBuffers.PositionVertexBuffer),
      _quadIndexBuffer(this->_numLines, this->_manualVertexFetchSupported),
      _pMaterial(InComponent->GetMaterial(0)),
      _materialRelevance(
          InSceneInterfaceParams.GetMaterialRelevance(InComponent)) {}

FCesiumGltfLinesSceneProxy::~FCesiumGltfLinesSceneProxy() {}

void FCesiumGltfLinesSceneProxy::CreateRenderThreadResources(
    FRHICommandListBase& RHICmdList) {
  this->_lineStyleVertexFactory.InitResource(RHICmdList);
  this->_quadIndexBuffer.InitResource(RHICmdList);

  if (this->shouldRenderWithLineWidth()) {
    const FRawStaticIndexBuffer& indexBuffer =
        this->_pRenderData->LODResources[0].IndexBuffer;
    bool b32Bit = indexBuffer.Is32Bit();
    this->_indexBufferSRV = RHICmdList.CreateShaderResourceView(
        indexBuffer.IndexBufferRHI,
        FRHIViewDesc::CreateBufferSRV()
            .SetType(FRHIViewDesc::EBufferType::Typed)
            .SetFormat(b32Bit ? PF_R32_UINT : PF_R16_UINT));
  }
}

void FCesiumGltfLinesSceneProxy::DestroyRenderThreadResources() {
  this->_lineStyleVertexFactory.ReleaseResource();
  this->_quadIndexBuffer.ReleaseResource();
  this->_indexBufferSRV.SafeRelease();
}

void FCesiumGltfLinesSceneProxy::DrawStaticElements(
    FStaticPrimitiveDrawInterface* PDI) {
  if (!HasViewDependentDPG()) {
    FMeshBatch Mesh;
    this->createMesh(Mesh);
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
      if (this->shouldRenderWithLineWidth()) {
        this->createMeshWithLineWidth(Mesh, View, Collector);
      } else {
        this->createMesh(Mesh);
      }
      Collector.AddMesh(ViewIndex, Mesh);
    }
  }
}

FPrimitiveViewRelevance
FCesiumGltfLinesSceneProxy::GetViewRelevance(const FSceneView* View) const {
  FPrimitiveViewRelevance Result;
  Result.bDrawRelevance = IsShown(View);

  if (this->shouldRenderWithLineWidth() || HasViewDependentDPG()) {
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

  this->_materialRelevance.SetPrimitiveViewRelevance(Result);

  return Result;
}

uint32 FCesiumGltfLinesSceneProxy::GetMemoryFootprint(void) const {
  return (sizeof(*this) + GetAllocatedSize());
}

void FCesiumGltfLinesSceneProxy::createLineStyleUserData(
    FMeshBatchElement& BatchElement,
    const FSceneView* View,
    FMeshElementCollector& Collector) const {
  FCesiumLineStyleBatchElementUserDataWrapper* pUserDataWrapper =
      &Collector.AllocateOneFrameResource<
          FCesiumLineStyleBatchElementUserDataWrapper>();

  FCesiumLineStyleBatchElementUserData& UserData = pUserDataWrapper->Data;
  const FLocalVertexFactory& OriginalVertexFactory =
      this->_pRenderData->LODVertexFactories[0].VertexFactory;

  UserData.IndexBuffer = this->_indexBufferSRV;
  UserData.PositionBuffer = OriginalVertexFactory.GetPositionsSRV();
  UserData.PackedTangentsBuffer = OriginalVertexFactory.GetTangentsSRV();
  UserData.ColorBuffer = OriginalVertexFactory.GetColorComponentsSRV();
  UserData.TexCoordBuffer = OriginalVertexFactory.GetTextureCoordinatesSRV();
  UserData.NumTexCoords = OriginalVertexFactory.GetNumTexcoords();
  UserData.bHasVertexColors =
      this->_pRenderData->LODResources[0].bHasColorVertexData;
  UserData.LineWidth = this->_lineWidth;
  UserData.Pattern = this->_pattern;
  BatchElement.UserData = &pUserDataWrapper->Data;
}

void FCesiumGltfLinesSceneProxy::createMeshWithLineWidth(
    FMeshBatch& Mesh,
    const FSceneView* View,
    FMeshElementCollector& Collector) const {
  Mesh.VertexFactory = &this->_lineStyleVertexFactory;
  Mesh.MaterialRenderProxy = this->_pMaterial->GetRenderProxy();
  Mesh.ReverseCulling = this->IsLocalToWorldDeterminantNegative();
  Mesh.Type = PT_TriangleList;
  Mesh.DepthPriorityGroup = SDPG_World;
  Mesh.LODIndex = 0;
  Mesh.bCanApplyViewModeOverrides = false;
  Mesh.bUseAsOccluder = false;
  Mesh.bWireframe = false;

  FMeshBatchElement& BatchElement = Mesh.Elements[0];
  BatchElement.IndexBuffer = &this->_quadIndexBuffer;
  BatchElement.NumPrimitives = this->_numLines * 2;
  BatchElement.FirstIndex = 0;
  BatchElement.MinVertexIndex = 0;
  BatchElement.MaxVertexIndex = this->_numLines * 4 - 1;
  BatchElement.PrimitiveUniformBuffer = this->GetUniformBuffer();

  this->createLineStyleUserData(BatchElement, View, Collector);
}

void FCesiumGltfLinesSceneProxy::createMesh(FMeshBatch& Mesh) const {
  Mesh.VertexFactory = &this->_pRenderData->LODVertexFactories[0].VertexFactory;
  Mesh.MaterialRenderProxy = this->_pMaterial->GetRenderProxy();
  Mesh.ReverseCulling = this->IsLocalToWorldDeterminantNegative();
  Mesh.Type = PT_LineList;
  Mesh.DepthPriorityGroup = SDPG_World;
  Mesh.LODIndex = 0;
  Mesh.bCanApplyViewModeOverrides = false;
  Mesh.bUseAsOccluder = false;
  Mesh.bWireframe = false;

  FMeshBatchElement& BatchElement = Mesh.Elements[0];
  BatchElement.IndexBuffer = &this->_pRenderData->LODResources[0].IndexBuffer;
  BatchElement.NumPrimitives = this->_numLines;
  BatchElement.FirstIndex = 0;
  BatchElement.MinVertexIndex = 0;
  BatchElement.MaxVertexIndex = BatchElement.NumPrimitives;
}

bool FCesiumGltfLinesSceneProxy::shouldRenderWithLineWidth() const {
  // The line width pipeline should be used if BENTLEY_materials_line_style is
  // present.
  return (this->_lineWidth >= 1 || this->_pattern != 0xFFFF) &&
         this->_manualVertexFetchSupported;
}
