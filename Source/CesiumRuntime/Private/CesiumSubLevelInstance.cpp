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

  if (IsValid(ResolvedGeoreference)) {
    UCesiumSubLevelSwitcherComponent* pSwitcher = this->_getSwitcher();
    if (pSwitcher)
      pSwitcher->RegisterSubLevel(this);
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
  this->ResolveGeoreference();
}

#if WITH_EDITOR

void ACesiumSubLevelInstance::SetIsTemporarilyHiddenInEditor(bool bIsHidden) {
  Super::SetIsTemporarilyHiddenInEditor(bIsHidden);

  this->ResolveGeoreference();
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
  this->ResolveGeoreference();
}

void ACesiumSubLevelInstance::PostActorCreated() {
  Super::PostActorCreated();

  // Set the initial location to (0,0,0).
  this->SetActorLocationAndRotation(FVector(0.0, 0.0, 0.0), FQuat::Identity);

  UCesiumSubLevelSwitcherComponent* pSwitcher = this->_getSwitcher();
  if (pSwitcher) {
    pSwitcher->OnCreateNewSubLevel(this);
  }
}

void ACesiumSubLevelInstance::BeginPlay() {
  Super::BeginPlay();
  this->ResolveGeoreference();
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
