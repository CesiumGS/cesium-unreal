// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumBoundingVolumeComponent.h"
#include "CesiumGeoreference.h"
#include "VecMath.h"
#include "UObject/UObjectGlobals.h"
#include <optional>
#include <variant>

using namespace Cesium3DTilesSelection;

TileOcclusionRendererProxy* 
UCesiumBoundingVolumePoolComponent::createProxy() {
  UCesiumBoundingVolumeComponent* pBoundingVolume = 
      NewObject<UCesiumBoundingVolumeComponent>(this);
  pBoundingVolume->SetVisibility(false);
  pBoundingVolume->bUseAsOccluder = false;
  pBoundingVolume->SetupAttachment(this);
  pBoundingVolume->RegisterComponent();
  return (TileOcclusionRendererProxy*)pBoundingVolume;
}

void 
UCesiumBoundingVolumePoolComponent::destroyProxy(
    TileOcclusionRendererProxy* pProxy) {
  // TODO: verify correctness of destruction
  //pProxy->BeginDestroy();
}

void UCesiumBoundingVolumePoolComponent::UpdateTransformFromCesium(
    const glm::dmat4& CesiumToUnrealTransform) {
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
  struct ConvertBoundingVolume {
    std::optional<FBoxSphereBounds>
    operator()(const CesiumGeometry::OrientedBoundingBox& box) {
      const glm::dmat3& halfAxes = box.getHalfAxes();
      FVector extent =
          VecMath::createVector(halfAxes[0] + halfAxes[1] + halfAxes[2]);
      return FBoxSphereBounds(FVector(), extent, extent.Size());
    }

    std::optional<FBoxSphereBounds>
    operator()(const CesiumGeometry::BoundingSphere& sphere) {
      float radius = static_cast<float>(sphere.getRadius());
      return FBoxSphereBounds(FVector(), FVector(radius), radius);
    }

    template <typename T>
    std::optional<FBoxSphereBounds> operator()(const T& other) {
      return std::nullopt;
    }
  };

public:
  FCesiumBoundingVolumeSceneProxy(
    const UCesiumBoundingVolumeComponent* pComponent,
    const BoundingVolume& localTileBounds) :
      FPrimitiveSceneProxy(pComponent/*, name?*/),
      _localTileBounds(localTileBounds) {}

  bool HasCustomOcclusionBounds() const override {
    return true;
  }

  FBoxSphereBounds GetCustomOcclusionBounds() const override {
    BoundingVolume transformedBoundingVolume = 
    transformBoundingVolume(
      VecMath::createMatrix4D(GetLocalToWorld()),
      this->_localTileBounds);

    std::optional<FBoxSphereBounds> bounds = 
        std::visit(ConvertBoundingVolume{}, transformedBoundingVolume);
    if (bounds) {
      return *bounds;
    }

    // TODO: find better default?
    return FBoxSphereBounds();
  }


  // TODO: IMPLEMENT THIS
  // this is almost certainly the key to removing _all_ current hacks 
  void AcceptOcclusionResults(
      const FSceneView* View, 
      TArray<bool>* Results, 
      int32 ResultsStart, 
      int32 NumResults) override {
    
  }

  /*/
  HHitProxy* CreateHitProxies(
      UPrimitiveComponent* Component,
      TArray<TRefCountPtr<HHitProxy>>& OutHitProxies) override {
    return nullptr;
  }*/

  SIZE_T GetTypeHash() const override {
    static size_t UniquePointer;
    return reinterpret_cast<size_t>(&UniquePointer);
  }

  uint32 GetMemoryFootprint(void) const override {
    return 0;
  }

private:
  BoundingVolume _localTileBounds;
};

FPrimitiveSceneProxy* UCesiumBoundingVolumeComponent::CreateSceneProxy() {
	return
      (FPrimitiveSceneProxy*)new FCesiumBoundingVolumeSceneProxy(
        this, 
        this->_localTileBounds);

}

UCesiumBoundingVolumeComponent::UCesiumBoundingVolumeComponent() 
    : _localTileBounds(CesiumGeometry::OrientedBoundingBox(
          glm::dvec3(0.0), glm::dmat3(1.0))),
      _tileTransform(1.0),
      _cesiumToUnreal(1.0) {}

void UCesiumBoundingVolumeComponent::SetOcclusionResult_RenderThread(
    const std::optional<bool>& isOccluded) {
  this->_isOccluded_RenderThread = isOccluded;
}

void UCesiumBoundingVolumeComponent::SyncOcclusionResult() {
  if (this->_isOccluded_RenderThread) {
    this->_isOccluded = *this->_isOccluded_RenderThread;
    this->_occlusionUpdatedThisFrame = true;
    this->_isOccluded_RenderThread = std::nullopt;
  }
}

void UCesiumBoundingVolumeComponent::_updateTransform() {
  this->SetUsingAbsoluteLocation(true);
  this->SetUsingAbsoluteRotation(true);
  this->SetUsingAbsoluteScale(true);

  const glm::dmat4x4& transform =
      this->_cesiumToUnreal * this->_tileTransform;

  this->SetRelativeTransform(FTransform(FMatrix(
      FVector(transform[0].x, transform[0].y, transform[0].z),
      FVector(transform[1].x, transform[1].y, transform[1].z),
      FVector(transform[2].x, transform[2].y, transform[2].z),
      FVector(transform[3].x, transform[3].y, transform[3].z))));
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
    // TODO: reject incompatible bounding volume types? Convert them all to
    // OBB / spheres?
    this->_tileTransform = pTile->getTransform();
    this->_localTileBounds = 
        transformBoundingVolume(
          glm::inverse(this->_tileTransform),
          pTile->getBoundingVolume());
    this->_updateTransform();

    this->SetVisibility(true);
    // TODO: might have to mark render state dirty somewhere here
  } else {
    this->_isOccluded = false;
    this->_lastUpdatedFrame = -1000;
    this->SetVisibility(false);
  }
}

void UCesiumBoundingVolumeComponent::update(int32_t currentFrame) {
  if (this->_occlusionUpdatedThisFrame) {
    this->_lastUpdatedFrame = currentFrame;
    this->_occlusionUpdatedThisFrame = false;
  }
}
