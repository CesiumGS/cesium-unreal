// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumBoundingVolumeComponent.h"
#include "CalcBounds.h"
#include "CesiumGeoreference.h"
#include "CesiumLifetime.h"
#include "UObject/UObjectGlobals.h"
#include "VecMath.h"
#include <optional>
#include <variant>
#include "Runtime/Renderer/Private/ScenePrivate.h"

using namespace Cesium3DTilesSelection;

UCesiumBoundingVolumePoolComponent::UCesiumBoundingVolumePoolComponent()
    : _cesiumToUnreal(1.0) {
  this->_pPool = std::make_shared<UCesiumBoundingVolumePool>(this);
  SetMobility(EComponentMobility::Static);
}

TileOcclusionRendererProxy* UCesiumBoundingVolumePoolComponent::createProxy() {
  UCesiumBoundingVolumeComponent* pBoundingVolume =
      NewObject<UCesiumBoundingVolumeComponent>(this);
  pBoundingVolume->SetVisibility(false);
  pBoundingVolume->bUseAsOccluder = false;
  pBoundingVolume->SetUsingAbsoluteLocation(true);

  // TODO: check if we can get away with this, may need to explicitly recreate
  // the scene proxy when georeference changes, reassigned to a different tile,
  // etc.
  pBoundingVolume->SetMobility(EComponentMobility::Static);
  pBoundingVolume->SetFlags(
      RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
  pBoundingVolume->SetupAttachment(this);
  pBoundingVolume->RegisterComponent();

  pBoundingVolume->UpdateTransformFromCesium(this->_cesiumToUnreal);

  return (TileOcclusionRendererProxy*)pBoundingVolume;
}

void UCesiumBoundingVolumePoolComponent::destroyProxy(
    TileOcclusionRendererProxy* pProxy) {
  // TODO: verify correctness of destruction
  UCesiumBoundingVolumeComponent* pBoundingVolumeComponent =
      (UCesiumBoundingVolumeComponent*)pProxy;
  if (pBoundingVolumeComponent) {
    CesiumLifetime::destroy(pBoundingVolumeComponent);
  }
}

UCesiumBoundingVolumePool::UCesiumBoundingVolumePool(
    UCesiumBoundingVolumePoolComponent* pOutter)
    : _pOutter(pOutter) {}

TileOcclusionRendererProxy* UCesiumBoundingVolumePool::createProxy() {
  return this->_pOutter->createProxy();
}

void UCesiumBoundingVolumePool::destroyProxy(
    TileOcclusionRendererProxy* pProxy) {
  this->_pOutter->destroyProxy(pProxy);
}

void UCesiumBoundingVolumePoolComponent::UpdateTransformFromCesium(
    const glm::dmat4& CesiumToUnrealTransform) {
  this->_cesiumToUnreal = CesiumToUnrealTransform;

  TArray<USceneComponent*> children = this->GetAttachChildren();
  for (USceneComponent* pChild : children) {
    UCesiumBoundingVolumeComponent* pBoundingVolume =
        Cast<UCesiumBoundingVolumeComponent>(pChild);
    if (pBoundingVolume) {
      pBoundingVolume->UpdateTransformFromCesium(CesiumToUnrealTransform);
    }
  }
}

class FCesiumBoundingVolumeSceneProxy : public FPrimitiveSceneProxy {
private:
  // TODO: FIND ANOTHER WAY, THIS IS NOT SAFE!!
  mutable UCesiumBoundingVolumeComponent* _pBoundingVolumeComponent;
public:
  FCesiumBoundingVolumeSceneProxy(UCesiumBoundingVolumeComponent* pComponent)
      : FPrimitiveSceneProxy(pComponent /*, name?*/),
        _pBoundingVolumeComponent(pComponent) {}

  bool AllowApproximateOcclusion() const {
    return true;
  }

  void GetDynamicMeshElements(
      const TArray<const FSceneView*>& Views,
      const FSceneViewFamily& ViewFamily,
      uint32 VisibilityMap,
      FMeshElementCollector& Collector) const override {
    bool isDefinite = true;
    // TODO: this is a workaround, not correct
    bool isOccluded = true;
    for (const FSceneView* pView : Views) {
      const FSceneViewState* pViewState = (FSceneViewState*)pView->State;
      //->GetConcreteViewState();
      if (pViewState && pViewState->PrimitiveOcclusionHistorySet.Num()) {
        const FPrimitiveOcclusionHistory* pHistory =
            pViewState->PrimitiveOcclusionHistorySet.Find(
                FPrimitiveOcclusionHistoryKey(
                    this->GetPrimitiveComponentId(),
                    0));
        //int32 NumBufferedFrames = FOcclusionQueryHelpers::GetNumBufferedFrames(ViewFamily.Scene->GetFeatureLevel());
        bool bGrouped = false;
        
        if (pHistory) {
          FRHIRenderQuery* query = pHistory->GetQueryForReading(
              pViewState->OcclusionFrameCounter,
              1,
              1,
              bGrouped);

          if (query) {

            uint64 NumSamples = 0;
            if (GDynamicRHI->RHIGetRenderQueryResult(query, NumSamples, true)) {
              isOccluded = isOccluded && (NumSamples == 0);
            }

            // isDefinite &= pHistory->OcclusionStateWasDefiniteLastFrame;
            // TODO: this is a hack, not correct
            // pHistory->WasOccludedLastFrame;
            // pHistory->LastPixelsPercentage < 0.1f; // WasOccludedLastFrame;
            /** / if (!pHistory->WasOccludedLastFrame) {
              isOccluded = false;
              break;
            }*/
          }
        }
      }
    }

    if (isDefinite) {
      this->_pBoundingVolumeComponent->SetOcclusionResult_RenderThread({isOccluded});
    }

    for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++) {
      const ESceneDepthPriorityGroup DrawBoundsDPG = SDPG_World;
      DrawWireBox(
          Collector.GetPDI(ViewIndex),
          GetBounds().GetBox(),
          FColor(72, 72, 255),
          DrawBoundsDPG);
      /*!Owner || IsSelected()*/
    }
  }

  FPrimitiveViewRelevance
  GetViewRelevance(const FSceneView* View) const override {
    FPrimitiveViewRelevance Result;
    
    Result.bDrawRelevance = true;
    Result.bDynamicRelevance = true;
    // Result.bEditorPrimitiveRelevance = true;
    return Result;
  }

  SIZE_T GetTypeHash() const override {
    static size_t UniquePointer;
    return reinterpret_cast<size_t>(&UniquePointer);
  }

  uint32 GetMemoryFootprint(void) const override {
    // Is this reasonable??
    return sizeof(FCesiumBoundingVolumeSceneProxy);
  }
};

FPrimitiveSceneProxy* UCesiumBoundingVolumeComponent::CreateSceneProxy() {
  this->_isOccluded = false;
  this->_isOccluded_RenderThread = false;
  this->_lastUpdatedFrame = -1000;
  return (FPrimitiveSceneProxy*)new FCesiumBoundingVolumeSceneProxy(this);
}

UCesiumBoundingVolumeComponent::UCesiumBoundingVolumeComponent()
    : _tileBounds(CesiumGeometry::OrientedBoundingBox(
          glm::dvec3(0.0),
          glm::dmat3(1.0))),
      _tileTransform(1.0),
      _cesiumToUnreal(1.0) {}

void UCesiumBoundingVolumeComponent::SetOcclusionResult_RenderThread(bool isOccluded) {
  this->_isOccluded_RenderThread = isOccluded;
}

void UCesiumBoundingVolumeComponent::_updateTransform() {
  this->SetUsingAbsoluteLocation(true);
  this->SetUsingAbsoluteRotation(true);
  this->SetUsingAbsoluteScale(true);

  const FTransform transform = FTransform(
      VecMath::createMatrix(this->_cesiumToUnreal * this->_tileTransform));

  this->SetRelativeTransform_Direct(transform);
  this->SetComponentToWorld(transform);
  this->MarkRenderTransformDirty();
}

void UCesiumBoundingVolumeComponent::UpdateTransformFromCesium(
    const glm::dmat4& CesiumToUnrealTransform) {
  this->_cesiumToUnreal = CesiumToUnrealTransform;
  this->_updateTransform();
}

bool UCesiumBoundingVolumeComponent::isOccluded() const {
  return this->_isOccluded;
}

int32_t UCesiumBoundingVolumeComponent::getLastUpdatedFrame() const {
  return this->_lastUpdatedFrame;
}

void UCesiumBoundingVolumeComponent::reset(const Tile* pTile) {
  if (pTile) {
    this->_tileTransform = pTile->getTransform();
    this->_tileBounds = pTile->getBoundingVolume();
    this->_updateTransform();
    this->SetVisibility(true);
    // TODO: might have to mark render state dirty somewhere here
  } else {
    this->SetVisibility(false);
  }
}

void UCesiumBoundingVolumeComponent::update(int32_t currentFrame) {
  this->_lastUpdatedFrame = currentFrame;
  this->_isOccluded = this->_isOccluded_RenderThread;
}

FBoxSphereBounds UCesiumBoundingVolumeComponent::CalcBounds(
    const FTransform& LocalToWorld) const {
  return std::visit(
      CalcBoundsOperation{LocalToWorld, this->_tileTransform},
      this->_tileBounds);
}
