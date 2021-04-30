// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "Cesium3DTilesetRoot.h"
#include "Cesium3DTileset.h"
#include "CesiumUtility/Math.h"
#include "Engine/World.h"

UCesium3DTilesetRoot::UCesium3DTilesetRoot()
    : _worldOriginLocation(0.0),
      _absoluteLocation(0.0, 0.0, 0.0),
      _tilesetToUnrealRelativeWorld(1.0),
      _isDirty(false) {
  PrimaryComponentTick.bCanEverTick = false;
}

void UCesium3DTilesetRoot::ApplyWorldOffset(
    const FVector& InOffset,
    bool bWorldShift) {
  USceneComponent::ApplyWorldOffset(InOffset, bWorldShift);

  const FIntVector& oldOrigin = this->GetWorld()->OriginLocation;
  this->_worldOriginLocation = glm::dvec3(
      static_cast<double>(oldOrigin.X) - static_cast<double>(InOffset.X),
      static_cast<double>(oldOrigin.Y) - static_cast<double>(InOffset.Y),
      static_cast<double>(oldOrigin.Z) - static_cast<double>(InOffset.Z));

  // Do _not_ call _updateAbsoluteLocation. The absolute position doesn't change
  // with an origin rebase, and we'll lose precision if we update the absolute
  // location here.

  this->_updateTilesetToUnrealRelativeWorldTransform();
}

void UCesium3DTilesetRoot::MarkTransformUnchanged() { this->_isDirty = false; }

void UCesium3DTilesetRoot::RecalculateTransform() {
  this->_updateTilesetToUnrealRelativeWorldTransform();
}

void NotifyGeoreferenceUpdated() {
  this->_updateTiletToUnrealRelativeWorldTransform();
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
  this->_absoluteLocation = glm::dvec3(
      static_cast<double>(originLocation.X) +
          static_cast<double>(newLocation.X),
      static_cast<double>(originLocation.Y) +
          static_cast<double>(newLocation.Y),
      static_cast<double>(originLocation.Z) +
          static_cast<double>(newLocation.Z));
}

void UCesium3DTilesetRoot::_updateTilesetToUnrealRelativeWorldTransform() {
  ACesium3DTileset* pTileset = this->GetOwner<ACesium3DTileset>();
  if (!IsValid(pTileset->Georeference)) {
    this->_tilesetToUnrealRelativeWorld = glm::dmat4(1.0);
    this->_isDirty = true;
    return;
  }

  const glm::dmat4& ellipsoidCenteredToUnrealWorld =
      pTileset->Georeference->GetEllipsoidCenteredToUnrealWorldTransform();

  glm::dvec3 relativeLocation =
      this->_absoluteLocation - this->_worldOriginLocation;

  FMatrix tilesetActorToUeLocal =
      this->GetComponentToWorld().ToMatrixWithScale();
  glm::dmat4 ueAbsoluteToUeLocal = glm::dmat4(
      glm::dvec4(
          tilesetActorToUeLocal.M[0][0],
          tilesetActorToUeLocal.M[0][1],
          tilesetActorToUeLocal.M[0][2],
          tilesetActorToUeLocal.M[0][3]),
      glm::dvec4(
          tilesetActorToUeLocal.M[1][0],
          tilesetActorToUeLocal.M[1][1],
          tilesetActorToUeLocal.M[1][2],
          tilesetActorToUeLocal.M[1][3]),
      glm::dvec4(
          tilesetActorToUeLocal.M[2][0],
          tilesetActorToUeLocal.M[2][1],
          tilesetActorToUeLocal.M[2][2],
          tilesetActorToUeLocal.M[2][3]),
      glm::dvec4(relativeLocation, 1.0));

  this->_tilesetToUnrealRelativeWorld =
      ueAbsoluteToUeLocal * ellipsoidCenteredToUnrealWorld;

  this->_isDirty = true;
}
