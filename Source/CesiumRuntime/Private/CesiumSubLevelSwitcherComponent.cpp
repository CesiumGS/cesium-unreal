#include "CesiumSubLevelSwitcherComponent.h"
#include "CesiumSubLevelInstance.h"
#include "Engine/World.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

void UCesiumSubLevelSwitcherComponent::RegisterSubLevel(
    ACesiumSubLevelInstance* pSubLevel) noexcept {
  this->_sublevels.AddUnique(pSubLevel);
  this->_ensureZeroOrOneSubLevelsAreActive();
}

void UCesiumSubLevelSwitcherComponent::UnregisterSubLevel(
    ACesiumSubLevelInstance* pSubLevel) noexcept {
  // If this sub-level is active, deactivate it.
  if (this->_pTarget == pSubLevel)
    this->SetTarget(nullptr);

  this->_sublevels.Remove(pSubLevel);
}

const TArray<TWeakObjectPtr<ACesiumSubLevelInstance>>&
UCesiumSubLevelSwitcherComponent::GetRegisteredSubLevels() const noexcept {
  return this->_sublevels;
}

void UCesiumSubLevelSwitcherComponent::OnCreateNewSubLevel(
    ACesiumSubLevelInstance* pSubLevel) noexcept {
  ACesiumGeoreference* pGeoreference =
      Cast<ACesiumGeoreference>(this->GetOwner());
  if (pGeoreference) {
    // Copy the current georeference info into the newly-created sublevel.
    pSubLevel->OriginLongitude = pGeoreference->OriginLongitude;
    pSubLevel->OriginLatitude = pGeoreference->OriginLatitude;
    pSubLevel->OriginHeight = pGeoreference->OriginHeight;
  }
}

ACesiumSubLevelInstance*
UCesiumSubLevelSwitcherComponent::GetCurrent() const noexcept {
  return this->_pCurrent;
}

ACesiumSubLevelInstance*
UCesiumSubLevelSwitcherComponent::GetTarget() const noexcept {
  return this->_pTarget;
}

void UCesiumSubLevelSwitcherComponent::SetTarget(
    ACesiumSubLevelInstance* pLevelInstance) noexcept {
  if (this->_pTarget != nullptr) {
    this->_deactivateSubLevel(this->_pTarget);
  }

  if (pLevelInstance) {
    this->_activateSubLevel(pLevelInstance);
  }

  this->_pTarget = pLevelInstance;
  this->_pCurrent = pLevelInstance;
}

#if WITH_EDITOR

void UCesiumSubLevelSwitcherComponent::
    NotifySubLevelIsTemporarilyHiddenInEditorChanged(
        ACesiumSubLevelInstance* pLevelInstance,
        bool bIsHidden) {
  if (!bIsHidden) {
    this->SetTarget(pLevelInstance);
  }
}

#endif

void UCesiumSubLevelSwitcherComponent::TickComponent(
    float DeltaTime,
    enum ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction) {
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UCesiumSubLevelSwitcherComponent::_ensureZeroOrOneSubLevelsAreActive() {
  bool foundFirstVisible = false;
  for (int32 i = 0; i < this->_sublevels.Num(); ++i) {
    ACesiumSubLevelInstance* Current = this->_sublevels[i].Get();
    if (!IsValid(Current))
      continue;

    if (this->_isSubLevelActive(Current)) {
      if (!foundFirstVisible) {
        // Ignore the first visible level.
        foundFirstVisible = true;
      } else {
        // Deactivate additional sublevels.
        this->_deactivateSubLevel(Current);
      }
    }
  }
}

void UCesiumSubLevelSwitcherComponent::_deactivateSubLevel(
    ACesiumSubLevelInstance* SubLevel) {
#if WITH_EDITOR
  if (GEditor && IsValid(this->GetWorld()) &&
      !this->GetWorld()->IsGameWorld()) {
    SubLevel->SetIsTemporarilyHiddenInEditor(true);
    return;
  }
#endif

  SubLevel->UnloadLevelInstance();
}

void UCesiumSubLevelSwitcherComponent::_activateSubLevel(
    ACesiumSubLevelInstance* SubLevel) {
  // Apply the sub-level's origin to this georeference.
  ACesiumGeoreference* pGeoreference =
      Cast<ACesiumGeoreference>(this->GetOwner());
  if (pGeoreference) {
    pGeoreference->SetGeoreferenceOriginLongitudeLatitudeHeight(glm::dvec3(
        SubLevel->OriginLongitude,
        SubLevel->OriginLatitude,
        SubLevel->OriginHeight));
  }

#if WITH_EDITOR
  if (GEditor && IsValid(this->GetWorld()) &&
      !this->GetWorld()->IsGameWorld()) {
    if (SubLevel->IsTemporarilyHiddenInEditor(true)) {
      SubLevel->SetIsTemporarilyHiddenInEditor(false);
    }
    return;
  }
#endif

  // UWorld* pWorld = SubLevel->GetWorldAsset().Get();
  // const auto& streamingLevels = GetWorld()->GetStreamingLevels();
  // for (int32 i = 0; i < streamingLevels.Num(); ++i) {
  //   UWorld* pOther = streamingLevels[i]->GetWorldAsset().Get();
  //   if (pWorld == pOther) {
  //     pWorld = pOther;
  //   }
  // }
  SubLevel->LoadLevelInstance();
}

bool UCesiumSubLevelSwitcherComponent::_isSubLevelActive(
    ACesiumSubLevelInstance* SubLevel) const {
#if WITH_EDITOR
  if (GEditor && IsValid(this->GetWorld()) &&
      !SubLevel->GetWorld()->IsGameWorld()) {
    return !SubLevel->IsTemporarilyHiddenInEditor(true);
  }
#endif

  return SubLevel->IsLoaded();
}
