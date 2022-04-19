// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumBoundingVolumeComponent.h"
#include "CesiumGeoreference.h"
#include "VecMath.h"
#include "Components/ActorComponent.h"
#include "UObject/UObjectGlobals.h"
#include <optional>
#include <variant>

TileOcclusionRendererProxy* 
UCesiumBoundingVolumePoolComponent::createProxy() overre {
  UCesiumBoundingVolumeComponent* pBoundingVolume = 
      NewObject<UCesiumBoundingVolumeComponent>(this);
  pBoundingVolume->SetVisibility(false);
  return (TileOcclusionRendererProxy*)pBoundingVolume;
}

void 
UCesiumBoundingVolumePoolComponent::destroyProxy(
    TileOcclusionRendererProxy* pProxy) {
  // TODO: verify correctness of destruction
  pBoundingVolume->BeginDestroy();
}

class FCesiumBoundingVolumeSceneProxy : public FPrimitiveSceneProxy {
public:
  FCesiumBoundingVolumeSceneProxy(
    const UCesiumBoundingVolumeComponent* pComponent,
    const FBoxSphereBounds& localTileBounds) :
      FPrimitiveSceneProxy(pComponent/*, name?*/),
      _localTileBounds(localTileBounds) {}

  bool HasCustomOcclusionBounds() const override {
    return true;
  }

  FBoxSphereBounds GetCustomOcclusionBounds() const override {
    return _localTileBounds.TransformBy(GetLocalToWorld());
  }

private:
  FBoxSphereBounds _localTileBounds;
};

FPrimitiveSceneProxy* UCesiumBoundingVolumeComponent::CreateSceneProxy() {
	return 
      (FPrimitiveSceneProxy*)new FCesiumBoundingVolumeSceneProxy(
        this, 
        this->_tileBounds);
}

void UCesiumBoundingVolumeComponent::SetOcclusionResult_RenderThread(
    const std::optional<bool>& isOccluded) {
  this->_isOccluded_RenderThread = isOccluded;
}

void UCesiumBoundingVolumeComponent::SyncOcclusionResult() {
  if (this->_isOccluded_RenderThread) {
    this->_isOccluded = *this->isOccluded_RenderThread;
    this->_occlusionUpdatedThisFrame = true;
    this->_isOccluded_RenderThread = std::nullopt;
  }
}

void UCesiumBoundingVolumeComponent::UpdateTransformFromCesium(
    const glm::dmat4& CesiumToUnrealTransform) {
  this->SetUsingAbsoluteLocation(true);
  this->SetUsingAbsoluteRotation(true);
  this->SetUsingAbsoluteScale(true);

  const glm::dmat4x4& transform =
      CesiumToUnrealTransform * this->_tileTransform;

  this->SetRelativeTransform(FTransform(FMatrix(
      FVector(transform[0].x, transform[0].y, transform[0].z),
      FVector(transform[1].x, transform[1].y, transform[1].z),
      FVector(transform[2].x, transform[2].y, transform[2].z),
      FVector(transform[3].x, transform[3].y, transform[3].z))));
}

bool UCesiumBoundingVolumeComponent::isOccluded() const {
  return this->_isOccluded;
}

int32_t UCesiumBoundingVolumeComponent::getLastUpdatedFrame() const {
  return this->_lastUpdatedFrame;
}

void UCesiumBoundingVolumeComponent::reset(const Tile* pTile) {
  if (pTile) {
    struct ConvertBoundingVolume {
      std::optional<FBoxSphereBounds> operator()(
          const CesiumGeometry::OrientedBoundingBox& box) {
        const glm::dmat3& halfAxes = box.getHalfAxes();
        FVector extent = 
            VecMath::createVector(halfAxes[0] + halfAxes[1] + halfAxes[2]);
        return FBoxSphereBounds(FVector(), extent, extent.Size());
      } 

      std::optional<FBoxSphereBounds> operator()(
          const CesiumGeometry::BoundingSphere& sphere) {
        float radius = static_cast<float>(sphere.getRadius());
        return FBoxSphereBounds(FVector(), FVector(radius), radius);
      } 

      template<typename T>
      std::optional<FBoxSphereBounds> operator()(
          const T& other) {
        return std::nullopt;
      }
    };

    std::optional<FBoxSphereBounds> bounds = 
        std::visit(ConvertBoundingVolume{}, this->_tileBounds);
    if (!bounds) {
      // TODO: probably not the best way to deal with unsupported
      // bounding volume tiles
      this->_isOccluded = false;
      this->_lastUpdatedFrame = -1000;
      this->SetVisibility(false);
      return;
    }

    this->_tileBounds = *bounds;
    this->_tileTransform = pTile->getTransform();

    this->SetVisibility(true);
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

