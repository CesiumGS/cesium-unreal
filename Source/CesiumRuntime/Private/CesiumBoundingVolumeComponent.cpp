// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumBoundingVolumeComponent.h"
#include "CalcBounds.h"
#include "CesiumCommon.h"
#include "CesiumGeoreference.h"
#include "CesiumLifetime.h"
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

void UCesiumBoundingVolumeComponent::UpdateOcclusion(
    const CesiumViewExtension& cesiumViewExtension) {
  if (!_isMapped) {
    return;
  }


  TileOcclusionState occlusionState =
      cesiumViewExtension.getPrimitiveOcclusionState(
          this->GetPrimitiveSceneId(),
          _occlusionState == TileOcclusionState::Occluded,
          _mappedFrameTime);

  // If the occlusion result is unavailable, continue using the previous result.
  if (occlusionState != TileOcclusionState::OcclusionUnavailable) {
    _occlusionState = occlusionState;
  }
}

void UCesiumBoundingVolumeComponent::_updateTransform() {
  const FTransform transform =
      VecMath::createTransform(this->_cesiumToUnreal * this->_tileTransform);

  this->SetRelativeTransform_Direct(transform);
  this->SetComponentToWorld(transform);
  this->MarkRenderTransformDirty();
}

void UCesiumBoundingVolumeComponent::UpdateTransformFromCesium(
    const glm::dmat4& CesiumToUnrealTransform) {
  this->_cesiumToUnreal = CesiumToUnrealTransform;
  this->_updateTransform();
}

void UCesiumBoundingVolumeComponent::reset(const Tile* pTile) {
  if (pTile) {
    this->_tileTransform = pTile->getTransform();
    this->_tileBounds = pTile->getBoundingVolume();
    this->_isMapped = true;
    this->_mappedFrameTime = GetWorld()->GetRealTimeSeconds();
    this->_updateTransform();
    this->SetVisibility(true);
  } else {
    this->_occlusionState = TileOcclusionState::OcclusionUnavailable;
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
