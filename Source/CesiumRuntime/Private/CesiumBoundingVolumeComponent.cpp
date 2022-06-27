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
  this->_pPool = std::make_shared<CesiumBoundingVolumePool>(this);
  SetMobility(EComponentMobility::Movable);
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
  // TODO: verify correctness of destruction
  UCesiumBoundingVolumeComponent* pBoundingVolumeComponent =
      (UCesiumBoundingVolumeComponent*)pProxy;
  if (pBoundingVolumeComponent) {
    CesiumLifetime::destroy(pBoundingVolumeComponent);
  }
}

UCesiumBoundingVolumePoolComponent::CesiumBoundingVolumePool::
    CesiumBoundingVolumePool(UCesiumBoundingVolumePoolComponent* pOutter)
    : _pOutter(pOutter) {}

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

void UCesiumBoundingVolumeComponent::UpdateOcclusionFromView(
    const FSceneView* View) {
  if (_ignoreRemainingViews) {
    return;
  }

  const FSceneViewState* pViewState = View->State->GetConcreteViewState();
  if (pViewState && pViewState->PrimitiveOcclusionHistorySet.Num()) {
    const FPrimitiveOcclusionHistory* pHistory =
        pViewState->PrimitiveOcclusionHistorySet.Find(
            FPrimitiveOcclusionHistoryKey(this->ComponentId, 0));
    if (pHistory) {
      if (!pHistory->OcclusionStateWasDefiniteLastFrame) {
        _isDefiniteThisFrame = false;
        _ignoreRemainingViews = true;
        return;
      }

      _isDefiniteThisFrame = true;

      if (_isOccluded) {
        if (pHistory->LastPixelsPercentage > 0.01f) {
          _isOccludedThisFrame = false;
          _ignoreRemainingViews = true;
          return;
        }
      } else {
        if (!pHistory->WasOccludedLastFrame) {
          // pHistory->LastPixelsPercentage > 0.0f) {
          _isOccludedThisFrame = false;
          _ignoreRemainingViews = true;
          return;
        }
      }

      _isOccludedThisFrame = true;
    }
  }
}

void UCesiumBoundingVolumeComponent::FinalizeOcclusionResultForFrame() {
  if (_isDefiniteThisFrame) {
    _isOccluded = _isOccludedThisFrame;
    _isOcclusionAvailable = true;
  }

  _isOccludedThisFrame = false;
  _isDefiniteThisFrame = true;
  _ignoreRemainingViews = false;
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

bool UCesiumBoundingVolumeComponent::isOcclusionAvailable() const {
  return this->_isOcclusionAvailable;
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
