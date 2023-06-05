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

  // Ignore transient level instances, like those that are created when dragging
  // from Create Actors but before releasing the mouse button.
  if (IsValid(this->ResolvedGeoreference) && !this->HasAllFlags(RF_Transient)) {
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
    ACesiumGeoreference* pGeoreference = this->ResolveGeoreference();
    if (IsValid(pGeoreference)) {
      pGeoreference->NotifySubLevelVisibleInEditor(this);
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
}

void ACesiumSubLevelInstance::BeginPlay() {
  Super::BeginPlay();
  this->ResolveGeoreference();
}
