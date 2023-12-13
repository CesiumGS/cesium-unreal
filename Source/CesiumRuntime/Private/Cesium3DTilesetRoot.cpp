// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "Cesium3DTilesetRoot.h"
#include "Cesium3DTileset.h"
#include "CesiumRuntime.h"
#include "CesiumUtility/Math.h"
#include "Engine/World.h"
#include "VecMath.h"

UCesium3DTilesetRoot::UCesium3DTilesetRoot()
    : _absoluteLocation(0.0, 0.0, 0.0), _tilesetToUnrealRelativeWorld(1.0) {
  PrimaryComponentTick.bCanEverTick = false;
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
  this->_absoluteLocation = VecMath::createVector3D(newLocation);
}

void UCesium3DTilesetRoot::_updateTilesetToUnrealRelativeWorldTransform() {
  ACesium3DTileset* pTileset = this->GetOwner<ACesium3DTileset>();

  this->_tilesetToUnrealRelativeWorld = VecMath::createMatrix4D(
      pTileset->ResolveGeoreference()
          ->ComputeEarthCenteredEarthFixedToUnrealTransformation());

  pTileset->UpdateTransformFromCesium();
}
