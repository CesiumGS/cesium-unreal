// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumBoundingVolumeComponent.h"
#include "CalcBounds.h"
#include "CesiumGeoreference.h"
#include "CesiumLifetime.h"
#include "Runtime/Renderer/Private/ScenePrivate.h"
#include "SceneTypes.h"
#include "UObject/UObjectGlobals.h"
#include "VecMath.h"
#include <optional>
#include <variant>

using namespace Cesium3DTilesSelection;

UCesiumBoundingVolumePoolComponent::UCesiumBoundingVolumePoolComponent()
    : _cesiumToUnreal(1.0) {
  SetMobility(EComponentMobility::Movable);
}

void UCesiumBoundingVolumePoolComponent::initPool(int32 maxPoolSize) {
  this->_pPool = std::make_shared<CesiumBoundingVolumePool>(this, maxPoolSize);
}

TileOcclusionRendererProxy* UCesiumBoundingVolumePoolComponent::createProxy() {
  UCesiumBoundingVolumeComponent* pBoundingVolume =
      NewObject<UCesiumBoundingVolumeComponent>(this);
  pBoundingVolume->SetVisibility(false);
  pBoundingVolume->bUseAsOccluder = false;
  pBoundingVolume->SetUsingAbsoluteLocation(true);

  pBoundingVolume->SetMobility(EComponentMobility::Movable);
  pBoundingVolume->SetFlags(
      RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
  pBoundingVolume->SetupAttachment(this);
  pBoundingVolume->RegisterComponent();

  pBoundingVolume->UpdateTransformFromCesium(this->_cesiumToUnreal);

  return (TileOcclusionRendererProxy*)pBoundingVolume;
}

void UCesiumBoundingVolumePoolComponent::destroyProxy(
    TileOcclusionRendererProxy* pProxy) {
  UCesiumBoundingVolumeComponent* pBoundingVolumeComponent =
      (UCesiumBoundingVolumeComponent*)pProxy;
  if (pBoundingVolumeComponent) {
    CesiumLifetime::destroyComponentRecursively(pBoundingVolumeComponent);
  }
}

UCesiumBoundingVolumePoolComponent::CesiumBoundingVolumePool::
    CesiumBoundingVolumePool(
        UCesiumBoundingVolumePoolComponent* pOutter,
        int32 maxPoolSize)
    : TileOcclusionRendererProxyPool(maxPoolSize), _pOutter(pOutter) {}

TileOcclusionRendererProxy*
UCesiumBoundingVolumePoolComponent::CesiumBoundingVolumePool::createProxy() {
  return this->_pOutter->createProxy();
}

void UCesiumBoundingVolumePoolComponent::CesiumBoundingVolumePool::destroyProxy(
    TileOcclusionRendererProxy* pProxy) {
  this->_pOutter->destroyProxy(pProxy);
}

void UCesiumBoundingVolumePoolComponent::UpdateTransformFromCesium(
    const glm::dmat4& CesiumToUnrealTransform) {
  this->_cesiumToUnreal = CesiumToUnrealTransform;

  const TArray<USceneComponent*>& children = this->GetAttachChildren();
  for (USceneComponent* pChild : children) {
    UCesiumBoundingVolumeComponent* pBoundingVolume =
        Cast<UCesiumBoundingVolumeComponent>(pChild);
    if (pBoundingVolume) {
      pBoundingVolume->UpdateTransformFromCesium(CesiumToUnrealTransform);
    }
  }
}

class FCesiumBoundingVolumeSceneProxy : public FPrimitiveSceneProxy {
public:
  FCesiumBoundingVolumeSceneProxy(UCesiumBoundingVolumeComponent* pComponent)
      : FPrimitiveSceneProxy(pComponent /*, name?*/) {}
  SIZE_T GetTypeHash() const override {
    static size_t UniquePointer;
    return reinterpret_cast<size_t>(&UniquePointer);
  }

  uint32 GetMemoryFootprint(void) const override {
    return sizeof(FCesiumBoundingVolumeSceneProxy) + GetAllocatedSize();
  }
};

FPrimitiveSceneProxy* UCesiumBoundingVolumeComponent::CreateSceneProxy() {
  return new FCesiumBoundingVolumeSceneProxy(this);
}

UCesiumBoundingVolumeComponent::UCesiumBoundingVolumeComponent()
    : _tileBounds(CesiumGeometry::OrientedBoundingBox(
          glm::dvec3(0.0),
          glm::dmat3(1.0))),
      _tileTransform(1.0),
      _cesiumToUnreal(1.0) {}

void UCesiumBoundingVolumeComponent::AggregateOcclusionFromView_RenderThread(
    const FSceneView* View) {
  if (_ignoreRemainingViews_RenderThread) {
    return;
  }

  if (!View->State) {
    return;
  }

  const FSceneViewState* pViewState = View->State->GetConcreteViewState();
  if (pViewState && pViewState->PrimitiveOcclusionHistorySet.Num()) {
    const FPrimitiveOcclusionHistory* pHistory =
        pViewState->PrimitiveOcclusionHistorySet.Find(
            FPrimitiveOcclusionHistoryKey(this->ComponentId, 0));
    if (pHistory) {
      if (!pHistory->OcclusionStateWasDefiniteLastFrame) {
        _isDefiniteThisFrame_RenderThread = false;
        _ignoreRemainingViews_RenderThread = true;
        return;
      }

      _isDefiniteThisFrame_RenderThread = true;

      if (_isOccluded) {
        if (pHistory->LastPixelsPercentage > 0.01f) {
          _isOccludedThisFrame_RenderThread = false;
          _ignoreRemainingViews_RenderThread = true;
          return;
        }
      } else {
        if (!pHistory->WasOccludedLastFrame) {
          // pHistory->LastPixelsPercentage > 0.0f) {
          _isOccludedThisFrame_RenderThread = false;
          _ignoreRemainingViews_RenderThread = true;
          return;
        }
      }

      _isOccludedThisFrame_RenderThread = true;
    }
  }
}

void UCesiumBoundingVolumeComponent::FinalizeOcclusionResultForFrame() {
  if (_isDefiniteThisFrame_RenderThread) {
    _isOccluded = _isOccludedThisFrame_RenderThread;
    _isOcclusionAvailable = true;
  }

  _isOccludedThisFrame_RenderThread = false;
  _isDefiniteThisFrame_RenderThread = true;
  _ignoreRemainingViews_RenderThread = false;
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

Cesium3DTilesSelection::TileOcclusionState
UCesiumBoundingVolumeComponent::getOcclusionState() const {
  if (!this->_isOcclusionAvailable) {
    return Cesium3DTilesSelection::TileOcclusionState::OcclusionUnavailable;
  } else if (this->_isOccluded) {
    return Cesium3DTilesSelection::TileOcclusionState::Occluded;
  } else {
    return Cesium3DTilesSelection::TileOcclusionState::NotOccluded;
  }
}

void UCesiumBoundingVolumeComponent::reset(const Tile* pTile) {
  if (pTile) {
    this->_tileTransform = pTile->getTransform();
    this->_tileBounds = pTile->getBoundingVolume();
    this->_isMapped = true;
    this->_updateTransform();
    this->SetVisibility(true);
  } else {
    this->_isOccluded = false;
    this->_isOcclusionAvailable = false;
    this->_isMapped = false;
    this->SetVisibility(false);
  }
}

FBoxSphereBounds UCesiumBoundingVolumeComponent::CalcBounds(
    const FTransform& LocalToWorld) const {
  return std::visit(
      CalcBoundsOperation{LocalToWorld, this->_tileTransform},
      this->_tileBounds);
}
