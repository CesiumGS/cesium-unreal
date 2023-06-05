// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumSubLevelInstance.h"

ACesiumGeoreference* ACesiumSubLevelInstance::ResolveGeoreference() {
  if (IsValid(this->ResolvedGeoreference)) {
    return this->ResolvedGeoreference;
  }

  if (IsValid(this->Georeference)) {
    this->ResolvedGeoreference = this->Georeference;
  } else {
    this->ResolvedGeoreference =
        ACesiumGeoreference::GetDefaultGeoreference(this);
  }

  if (this->_isAttachedToGeoreference()) {
    this->ResolvedGeoreference->AddSubLevel(this);
  }

  return this->ResolvedGeoreference;
}

void ACesiumSubLevelInstance::InvalidateResolvedGeoreference() {
  if (this->ResolvedGeoreference) {
    this->ResolvedGeoreference->RemoveSubLevel(this);
  }
  this->ResolvedGeoreference = nullptr;
}

ACesiumGeoreference* ACesiumSubLevelInstance::GetGeoreference() const {
  return this->Georeference;
}

void ACesiumSubLevelInstance::SetGeoreference(
    ACesiumGeoreference* NewGeoreference) {
  this->Georeference = NewGeoreference;
  this->InvalidateResolvedGeoreference();
  this->ResolveGeoreference();
}

#if WITH_EDITOR

void ACesiumSubLevelInstance::SetIsTemporarilyHiddenInEditor(bool bIsHidden) {
  Super::SetIsTemporarilyHiddenInEditor(bIsHidden);

  // If this sub-level just became visible, notify the georeference so it can
  // hide the other levels.
  if (!bIsHidden) {
    this->ResolveGeoreference();
    if (this->_isAttachedToGeoreference()) {
      this->ResolvedGeoreference->ActivateSubLevel(this);
    }
  }
}

#endif

void ACesiumSubLevelInstance::BeginDestroy() {
  this->InvalidateResolvedGeoreference();
  Super::BeginDestroy();
}

void ACesiumSubLevelInstance::OnConstruction(const FTransform& Transform) {
  Super::OnConstruction(Transform);
  this->ResolveGeoreference();

  if (this->_isAttachedToGeoreference()) {
    this->ResolvedGeoreference->SyncSubLevel(this);
  }
}

void ACesiumSubLevelInstance::PostActorCreated() {
  Super::PostActorCreated();

  // When a new sub-level is created (not loaded!)...

  // Set the initial location to (0,0,0).
  this->SetActorLocationAndRotation(FVector(0.0, 0.0, 0.0), FQuat::Identity);

  // Copy the current georeference info into it.
  this->ResolveGeoreference();
  if (this->_isAttachedToGeoreference()) {
    this->OriginLongitude = this->ResolvedGeoreference->OriginLongitude;
    this->OriginLatitude = this->ResolvedGeoreference->OriginLatitude;
    this->OriginHeight = this->ResolvedGeoreference->OriginHeight;
  }
}

void ACesiumSubLevelInstance::BeginPlay() {
  Super::BeginPlay();
  this->ResolveGeoreference();

  if (this->_isAttachedToGeoreference()) {
    this->ResolvedGeoreference->SyncSubLevel(this);
  }
}

bool ACesiumSubLevelInstance::_isAttachedToGeoreference() const {
  // Ignore transient level instances, like those that are created when dragging
  // from Create Actors but before releasing the mouse button.
  return IsValid(this->ResolvedGeoreference) &&
         !this->HasAllFlags(RF_Transient);
}
