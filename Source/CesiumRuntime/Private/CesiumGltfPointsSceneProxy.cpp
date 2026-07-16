// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumGltfPointsSceneProxy.h"
#include "CesiumGltfPointsComponent.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "Engine/StaticMesh.h"
#include "RHIResources.h"
#include "Runtime/Launch/Resources/Version.h"
#include "SceneInterface.h"
#include "SceneView.h"
#include "StaticMeshResources.h"

FCesiumGltfPointsSceneProxyAttenuationData::
    FCesiumGltfPointsSceneProxyAttenuationData()
    : tilesetPointCloudShading(),
      maximumScreenSpaceError(0.0),
      usesAdditiveRefinement(false),
      geometricError(0.0f),
      dimensions(),
      diameter(0) {}

FCesiumGltfPointsSceneProxyAttenuationData::
    FCesiumGltfPointsSceneProxyAttenuationData(
        UCesiumGltfPointsComponent* pComponent) {
  if (!IsValid(pComponent)) {
    return;
  }

  this->usesAdditiveRefinement = pComponent->usesAdditiveRefinement;
  this->geometricError = pComponent->geometricError;
  this->dimensions = pComponent->dimensions;
  this->diameter = pComponent->diameter;

  if (ACesium3DTileset* pTileset = pComponent->getPrimitiveData().pTilesetActor;
      IsValid(pTileset)) {
    this->tilesetPointCloudShading = pTileset->GetPointCloudShading();
    this->maximumScreenSpaceError = pTileset->MaximumScreenSpaceError;
  }
}

SIZE_T FCesiumGltfPointsSceneProxy::GetTypeHash() const {
  static size_t UniquePointer;
  return reinterpret_cast<size_t>(&UniquePointer);
}

FCesiumGltfPointsSceneProxy::FCesiumGltfPointsSceneProxy(
    UCesiumGltfPointsComponent* InComponent,
    FSceneInterfaceWrapper InSceneInterfaceParams)
    : FPrimitiveSceneProxy(InComponent),
      _pRenderData(InComponent->GetStaticMesh()->GetRenderData()),
      _numPoints(
          this->_pRenderData->LODResources[0].IndexBuffer.GetNumIndices()),
      _attenuationSupported(
          RHISupportsManualVertexFetch(GetScene().GetShaderPlatform())),
      _attenuationData(),
      _attenuationVertexFactory(
          InSceneInterfaceParams.RHIFeatureLevelType,
          &this->_pRenderData->LODResources[0]
               .VertexBuffers.PositionVertexBuffer),
      _attenuationIndexBuffer(this->_numPoints, this->_attenuationSupported),
      _pMaterial(InComponent->GetMaterial(0)),
      _materialRelevance(
          InSceneInterfaceParams.GetMaterialRelevance(InComponent)) {}

FCesiumGltfPointsSceneProxy::~FCesiumGltfPointsSceneProxy() {}

void FCesiumGltfPointsSceneProxy::CreateRenderThreadResources(
    FRHICommandListBase& RHICmdList) {
  this->_attenuationVertexFactory.InitResource(RHICmdList);
  this->_attenuationIndexBuffer.InitResource(RHICmdList);
}

void FCesiumGltfPointsSceneProxy::DestroyRenderThreadResources() {
  this->_attenuationVertexFactory.ReleaseResource();
  this->_attenuationIndexBuffer.ReleaseResource();
}

void FCesiumGltfPointsSceneProxy::GetDynamicMeshElements(
    const TArray<const FSceneView*>& Views,
    const FSceneViewFamily& ViewFamily,
    uint32 VisibilityMap,
    FMeshElementCollector& Collector) const {
  QUICK_SCOPE_CYCLE_COUNTER(STAT_GltfPointsSceneProxy_GetDynamicMeshElements);

  // The attenuation pipeline should be used if BENTLEY_materials_point_style is
  // present.
  bool useAttenuation =
      this->_attenuationData.tilesetPointCloudShading.Attenuation ||
      this->_attenuationData.diameter >= 1;
  useAttenuation &= this->_attenuationSupported;

  for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++) {
    if (VisibilityMap & (1 << ViewIndex)) {
      const FSceneView* View = Views[ViewIndex];
      FMeshBatch& Mesh = Collector.AllocateMesh();
      if (useAttenuation) {
        CreateMeshWithAttenuation(Mesh, View, Collector);
      } else {
        CreateMesh(Mesh);
      }
      Collector.AddMesh(ViewIndex, Mesh);
    }
  }
}

FPrimitiveViewRelevance
FCesiumGltfPointsSceneProxy::GetViewRelevance(const FSceneView* View) const {
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
  Result.bVelocityRelevance =
      IsMovable() & Result.bOpaque & Result.bRenderInMainPass;

  this->_materialRelevance.SetPrimitiveViewRelevance(Result);

  return Result;
}

uint32 FCesiumGltfPointsSceneProxy::GetMemoryFootprint(void) const {
  return (sizeof(*this) + GetAllocatedSize());
}

void FCesiumGltfPointsSceneProxy::UpdateAttenuationData(
    const FCesiumGltfPointsSceneProxyAttenuationData& InAttenuationData) {
  this->_attenuationData = InAttenuationData;
}

float FCesiumGltfPointsSceneProxy::getGeometricError() const {
  FCesiumPointCloudShading pointCloudShading =
      this->_attenuationData.tilesetPointCloudShading;
  float geometricError = this->_attenuationData.geometricError;
  if (geometricError > 0.0f) {
    return geometricError;
  }

  if (pointCloudShading.BaseResolution > 0.0f) {
    return pointCloudShading.BaseResolution;
  }

  // Estimate the geometric error.
  glm::vec3 dimensions = this->_attenuationData.dimensions;
  float volume = dimensions.x * dimensions.y * dimensions.z;
  return FMath::Pow(volume / this->_numPoints, 1.0f / 3.0f);
}

void FCesiumGltfPointsSceneProxy::CreatePointAttenuationUserData(
    FMeshBatchElement& BatchElement,
    const FSceneView* View,
    FMeshElementCollector& Collector) const {
  FCesiumPointAttenuationBatchElementUserDataWrapper* UserDataWrapper =
      &Collector.AllocateOneFrameResource<
          FCesiumPointAttenuationBatchElementUserDataWrapper>();

  FCesiumPointAttenuationBatchElementUserData& UserData = UserDataWrapper->Data;
  const FLocalVertexFactory& OriginalVertexFactory =
      this->_pRenderData->LODVertexFactories[0].VertexFactory;

  UserData.PositionBuffer = OriginalVertexFactory.GetPositionsSRV();
  UserData.PackedTangentsBuffer = OriginalVertexFactory.GetTangentsSRV();
  UserData.ColorBuffer = OriginalVertexFactory.GetColorComponentsSRV();
  UserData.TexCoordBuffer = OriginalVertexFactory.GetTextureCoordinatesSRV();
  UserData.NumTexCoords = OriginalVertexFactory.GetNumTexcoords();
  UserData.bHasPointColors =
      this->_pRenderData->LODResources[0].bHasColorVertexData;

  FCesiumPointCloudShading pointCloudShading =
      this->_attenuationData.tilesetPointCloudShading;

  float maximumPointSize = 1.0f;

  if (pointCloudShading.Attenuation) {
    maximumPointSize = this->_attenuationData.usesAdditiveRefinement
                           ? 5.0f
                           : this->_attenuationData.maximumScreenSpaceError;
    if (pointCloudShading.MaximumAttenuation > 0.0f) {
      // Don't multiply by DPI scale; let Unreal handle scaling.
      maximumPointSize = pointCloudShading.MaximumAttenuation;
    }
  }

  float geometricError = this->getGeometricError();
  geometricError *= pointCloudShading.GeometricErrorScale;

  // Depth Multiplier
  float sseDenominator = 2.0f * tanf(0.5f * FMath::DegreesToRadians(View->FOV));
  float depthMultiplier =
      static_cast<float>(View->UnconstrainedViewRect.Height()) / sseDenominator;

  int32 diameter = this->_attenuationData.diameter;

  UserData.AttenuationParameters =
      FVector4f(maximumPointSize, geometricError, depthMultiplier, diameter);
  BatchElement.UserData = &UserDataWrapper->Data;
}

void FCesiumGltfPointsSceneProxy::CreateMeshWithAttenuation(
    FMeshBatch& Mesh,
    const FSceneView* View,
    FMeshElementCollector& Collector) const {
  Mesh.VertexFactory = &this->_attenuationVertexFactory;
  Mesh.MaterialRenderProxy = this->_pMaterial->GetRenderProxy();
  Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
  Mesh.Type = PT_TriangleList;
  Mesh.DepthPriorityGroup = SDPG_World;
  Mesh.LODIndex = 0;
  Mesh.bCanApplyViewModeOverrides = false;
  Mesh.bUseAsOccluder = false;
  Mesh.bWireframe = false;

  FMeshBatchElement& BatchElement = Mesh.Elements[0];
  BatchElement.IndexBuffer = &this->_attenuationIndexBuffer;
  BatchElement.NumPrimitives = this->_numPoints * 2;
  BatchElement.FirstIndex = 0;
  BatchElement.MinVertexIndex = 0;
  BatchElement.MaxVertexIndex = this->_numPoints * 4 - 1;
  BatchElement.PrimitiveUniformBuffer = GetUniformBuffer();

  CreatePointAttenuationUserData(BatchElement, View, Collector);
}

void FCesiumGltfPointsSceneProxy::CreateMesh(FMeshBatch& Mesh) const {
  Mesh.VertexFactory = &this->_pRenderData->LODVertexFactories[0].VertexFactory;
  Mesh.MaterialRenderProxy = this->_pMaterial->GetRenderProxy();
  Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
  Mesh.Type = PT_PointList;
  Mesh.DepthPriorityGroup = SDPG_World;
  Mesh.LODIndex = 0;
  Mesh.bCanApplyViewModeOverrides = false;
  Mesh.bUseAsOccluder = false;
  Mesh.bWireframe = false;

  FMeshBatchElement& BatchElement = Mesh.Elements[0];
  BatchElement.IndexBuffer = &this->_pRenderData->LODResources[0].IndexBuffer;
  BatchElement.NumPrimitives = this->_numPoints;
  BatchElement.FirstIndex = 0;
  BatchElement.MinVertexIndex = 0;
  BatchElement.MaxVertexIndex = BatchElement.NumPrimitives - 1;
}
