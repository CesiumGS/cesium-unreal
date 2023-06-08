// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumSubLevelInstance.h"
#include "CesiumSubLevelSwitcherComponent.h"

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

  return this->ResolvedGeoreference;
}

void ACesiumSubLevelInstance::InvalidateResolvedGeoreference() {
  if (this->ResolvedGeoreference) {
    UCesiumSubLevelSwitcherComponent* pSwitcher = this->_getSwitcher();
    if (pSwitcher)
      pSwitcher->UnregisterSubLevel(this);
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

  UCesiumSubLevelSwitcherComponent* pSwitcher = this->_getSwitcher();
  if (pSwitcher)
    pSwitcher->RegisterSubLevel(this);
}

void ACesiumSubLevelInstance::ActivateSubLevel() {
  // Apply the sub-level's origin to this georeference, if it's different.
  if (IsValid(this->ResolvedGeoreference)) {
    if (this->OriginLongitude != this->ResolvedGeoreference->OriginLongitude ||
        this->OriginLatitude != this->ResolvedGeoreference->OriginLatitude ||
        this->OriginHeight != this->ResolvedGeoreference->OriginHeight) {
      this->ResolvedGeoreference->SetGeoreferenceOriginLongitudeLatitudeHeight(
          glm::dvec3(
              this->OriginLongitude,
              this->OriginLatitude,
              this->OriginHeight));
    }
  }
}

#if WITH_EDITOR

void ACesiumSubLevelInstance::SetIsTemporarilyHiddenInEditor(bool bIsHidden) {
  Super::SetIsTemporarilyHiddenInEditor(bIsHidden);

  UCesiumSubLevelSwitcherComponent* pSwitcher = this->_getSwitcher();
  if (pSwitcher) {
    pSwitcher->NotifySubLevelIsTemporarilyHiddenInEditorChanged(
        this,
        bIsHidden);
  }
}

#endif

void ACesiumSubLevelInstance::BeginDestroy() {
  this->InvalidateResolvedGeoreference();
  Super::BeginDestroy();
}

void ACesiumSubLevelInstance::OnConstruction(const FTransform& Transform) {
  Super::OnConstruction(Transform);

  UCesiumSubLevelSwitcherComponent* pSwitcher = this->_getSwitcher();
  if (pSwitcher)
    pSwitcher->RegisterSubLevel(this);
}

void ACesiumSubLevelInstance::PostActorCreated() {
  Super::PostActorCreated();

  // Set the initial location to (0,0,0).
  this->SetActorLocationAndRotation(FVector(0.0, 0.0, 0.0), FQuat::Identity);

  UCesiumSubLevelSwitcherComponent* pSwitcher = this->_getSwitcher();
  if (pSwitcher && this->ResolvedGeoreference) {
    // Copy the current georeference info into the newly-created sublevel.
    this->OriginLongitude = this->ResolvedGeoreference->OriginLongitude;
    this->OriginLatitude = this->ResolvedGeoreference->OriginLatitude;
    this->OriginHeight = this->ResolvedGeoreference->OriginHeight;
  }
}

void ACesiumSubLevelInstance::BeginPlay() {
  Super::BeginPlay();

  UCesiumSubLevelSwitcherComponent* pSwitcher = this->_getSwitcher();
  if (pSwitcher)
    pSwitcher->RegisterSubLevel(this);
}

UCesiumSubLevelSwitcherComponent*
ACesiumSubLevelInstance::_getSwitcher() noexcept {
  ACesiumGeoreference* pGeoreference = this->ResolveGeoreference();

  // Ignore transient level instances, like those that are created when dragging
  // from Create Actors but before releasing the mouse button.
  if (!IsValid(pGeoreference) || this->HasAllFlags(RF_Transient))
    return nullptr;

  return pGeoreference
      ->FindComponentByClass<UCesiumSubLevelSwitcherComponent>();
}
