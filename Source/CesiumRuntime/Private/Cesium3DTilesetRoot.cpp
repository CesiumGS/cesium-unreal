// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "Cesium3DTilesetRoot.h"
#include "Cesium3DTileset.h"
#include "CesiumRuntime.h"
#include "CesiumUtility/Math.h"
#include "Engine/World.h"
#include "VecMath.h"

UCesium3DTilesetRoot::UCesium3DTilesetRoot()
    : _worldOriginLocation(0.0),
      _absoluteLocation(0.0, 0.0, 0.0),
      _tilesetToUnrealRelativeWorld(1.0) {
  PrimaryComponentTick.bCanEverTick = false;
}

void UCesium3DTilesetRoot::ApplyWorldOffset(
    const FVector& InOffset,
    bool bWorldShift) {
  USceneComponent::ApplyWorldOffset(InOffset, bWorldShift);

  const FIntVector& oldOrigin = this->GetWorld()->OriginLocation;
  this->_worldOriginLocation = VecMath::subtract3D(oldOrigin, InOffset);

  // Do _not_ call _updateAbsoluteLocation. The absolute position doesn't change
  // with an origin rebase, and we'll lose precision if we update the absolute
  // location here.

  this->_updateTilesetToUnrealRelativeWorldTransform();
}

void UCesium3DTilesetRoot::HandleGeoreferenceUpdated() {
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("Called HandleGeoreferenceUpdated for tileset root %s"),
      *this->GetName());
  this->_updateTilesetToUnrealRelativeWorldTransform();
}

const glm::dmat4&
UCesium3DTilesetRoot::GetCesiumTilesetToUnrealRelativeWorldTransform() const {
  return this->_tilesetToUnrealRelativeWorld;
}

// Called when the game starts
void UCesium3DTilesetRoot::BeginPlay() {
  Super::BeginPlay();

  this->_updateAbsoluteLocation();
  this->_updateTilesetToUnrealRelativeWorldTransform();
}

bool UCesium3DTilesetRoot::MoveComponentImpl(
    const FVector& Delta,
    const FQuat& NewRotation,
    bool bSweep,
    FHitResult* OutHit,
    EMoveComponentFlags MoveFlags,
    ETeleportType Teleport) {
  bool result = USceneComponent::MoveComponentImpl(
      Delta,
      NewRotation,
      bSweep,
      OutHit,
      MoveFlags,
      Teleport);

  this->_updateAbsoluteLocation();
  this->_updateTilesetToUnrealRelativeWorldTransform();

  return result;
}

void UCesium3DTilesetRoot::_updateAbsoluteLocation() {
  const FVector& newLocation = this->GetRelativeLocation();
  const FIntVector& originLocation = this->GetWorld()->OriginLocation;
  this->_absoluteLocation = VecMath::add3D(originLocation, newLocation);
  this->_worldOriginLocation = VecMath::createVector3D(originLocation);
}

void UCesium3DTilesetRoot::_updateTilesetToUnrealRelativeWorldTransform() {
  ACesium3DTileset* pTileset = this->GetOwner<ACesium3DTileset>();

  const glm::dmat4& ellipsoidCenteredToUnrealWorld =
      pTileset->ResolveGeoreference()
          ->GetGeoTransforms()
          .GetEllipsoidCenteredToAbsoluteUnrealWorldTransform();

  glm::dvec3 relativeLocation =
      this->_absoluteLocation - this->_worldOriginLocation;

  FMatrix tilesetActorToUeLocal =
      this->GetComponentToWorld().ToMatrixWithScale();
  glm::dmat4 ueAbsoluteToUeLocal =
      VecMath::createMatrix4D(tilesetActorToUeLocal, relativeLocation);

  this->_tilesetToUnrealRelativeWorld =
      ueAbsoluteToUeLocal * ellipsoidCenteredToUnrealWorld;

  pTileset->UpdateTransformFromCesium();
}
