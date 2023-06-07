#include "CesiumSubLevelSwitcherComponent.h"
#include "CesiumSubLevelInstance.h"
#include "Engine/LevelStreaming.h"
#include "Engine/World.h"
#include "LevelInstance/LevelInstanceLevelStreaming.h"
#include "LevelInstance/LevelInstanceSubsystem.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

UCesiumSubLevelSwitcherComponent::UCesiumSubLevelSwitcherComponent() {
  this->PrimaryComponentTick.bCanEverTick = true;
}

void UCesiumSubLevelSwitcherComponent::RegisterSubLevel(
    ACesiumSubLevelInstance* pSubLevel) noexcept {
  this->_sublevels.AddUnique(pSubLevel);

  // Levels should initially be unloaded.
  pSubLevel->UnloadLevelInstance();

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
  this->_pTarget = pLevelInstance;

  // If there is no other sublevels currently active, move the georeference
  // immediately.
  if (this->_pCurrent == nullptr && this->_pTarget != nullptr) {
    this->_pTarget->ActivateSubLevel();
  }
}

#if WITH_EDITOR

void UCesiumSubLevelSwitcherComponent::
    NotifySubLevelIsTemporarilyHiddenInEditorChanged(
        ACesiumSubLevelInstance* pLevelInstance,
        bool bIsHidden) {
  if (bIsHidden) {
    // The previous target level has been hidden, so clear out the target.
    if (this->_pTarget == pLevelInstance) {
      this->_pTarget = nullptr;
    }
  } else {
    this->SetTarget(pLevelInstance);
  }
}

#endif

void UCesiumSubLevelSwitcherComponent::TickComponent(
    float DeltaTime,
    enum ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction) {
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  if (this->_pTarget == this->_pCurrent) {
    // We already match the desired state, so there's nothing to do!
    return;
  }

  if (this->_pCurrent != nullptr) {
    // Work toward unloading the current level.

    ULevelStreaming* pStreaming =
        this->_getLevelStreamingForSubLevel(this->_pCurrent);

    ULevelStreaming::ECurrentState state =
        ULevelStreaming::ECurrentState::Unloaded;
    if (IsValid(pStreaming)) {
      state = pStreaming->GetCurrentState();
    } else if (this->_pCurrent->GetWorldAsset().IsNull()) {
      // There is no level associated with the target at all, so mark it
      // unloaded but also deactivate it for the benefit of the Editor UI.
      this->_deactivateSubLevel(this->_pCurrent);
    }

    switch (state) {
    case ULevelStreaming::ECurrentState::Loading:
    case ULevelStreaming::ECurrentState::MakingInvisible:
    case ULevelStreaming::ECurrentState::MakingVisible:
      // Wait for these transitions to finish before doing anything further.
      // TODO: maybe we can cancel these transitions somehow?
      break;
    case ULevelStreaming::ECurrentState::FailedToLoad:
    case ULevelStreaming::ECurrentState::LoadedNotVisible:
    case ULevelStreaming::ECurrentState::LoadedVisible:
      this->_deactivateSubLevel(this->_pCurrent);
      break;
    case ULevelStreaming::ECurrentState::Removed:
    case ULevelStreaming::ECurrentState::Unloaded:
      this->_pCurrent = nullptr;

      // Now that no other sub-levels are active, it's safe to set the
      // georeference to the new target location.
      if (this->_pTarget) {
        this->_pTarget->ActivateSubLevel();
      }
      break;
    }
  }

  if (this->_pCurrent == nullptr && this->_pTarget != nullptr) {
    // Once the current level is unloaded, work toward loading the target level.

    ULevelStreaming* pStreaming =
        this->_getLevelStreamingForSubLevel(this->_pTarget);

    ULevelStreaming::ECurrentState state =
        ULevelStreaming::ECurrentState::Unloaded;
    if (IsValid(pStreaming)) {
      state = pStreaming->GetCurrentState();
    } else if (this->_pTarget->GetWorldAsset().IsNull()) {
      // There is no level associated with the target at all, so mark it failed
      // to load, but also activate it for the benefit of the Editor UI.
      state = ULevelStreaming::ECurrentState::FailedToLoad;
      this->_activateSubLevel(this->_pTarget);
    }

    switch (state) {
    case ULevelStreaming::ECurrentState::Loading:
    case ULevelStreaming::ECurrentState::MakingInvisible:
    case ULevelStreaming::ECurrentState::MakingVisible:
      // Wait for these transitions to finish before doing anything further.
      break;
    case ULevelStreaming::ECurrentState::FailedToLoad:
    case ULevelStreaming::ECurrentState::LoadedNotVisible:
    case ULevelStreaming::ECurrentState::LoadedVisible:
      // Loading complete!
      this->_pCurrent = this->_pTarget;
      break;
    case ULevelStreaming::ECurrentState::Removed:
    case ULevelStreaming::ECurrentState::Unloaded:
      this->_activateSubLevel(this->_pTarget);
      break;
    }
  }
}

void UCesiumSubLevelSwitcherComponent::_ensureZeroOrOneSubLevelsAreActive() {
  bool foundFirstVisible = false;
  for (int32 i = 0; i < this->_sublevels.Num(); ++i) {
    ACesiumSubLevelInstance* Current = this->_sublevels[i].Get();
    if (!IsValid(Current))
      continue;

    if (this->_isSubLevelActive(Current)) {
      if (!foundFirstVisible) {
        // Make the first visible level the target.
        foundFirstVisible = true;
        this->SetTarget(Current);
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
  }
#endif

  SubLevel->UnloadLevelInstance();
}

void UCesiumSubLevelSwitcherComponent::_activateSubLevel(
    ACesiumSubLevelInstance* SubLevel) {
#if WITH_EDITOR
  if (GEditor && IsValid(this->GetWorld()) &&
      !this->GetWorld()->IsGameWorld()) {
    if (SubLevel->IsTemporarilyHiddenInEditor(true)) {
      SubLevel->SetIsTemporarilyHiddenInEditor(false);
    }
  }
#endif

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

  return SubLevel->GetLevelInstanceSubsystem()->IsLoaded(SubLevel);
}

ULevelStreaming*
UCesiumSubLevelSwitcherComponent::_getLevelStreamingForSubLevel(
    ACesiumSubLevelInstance* SubLevel) const {
  if (!IsValid(SubLevel))
    return nullptr;

  ULevelStreaming* const* ppStreaming =
      GetWorld()->GetStreamingLevels().FindByPredicate(
          [SubLevel](ULevelStreaming* pStreaming) {
            ULevelStreamingLevelInstance* pInstanceStreaming =
                Cast<ULevelStreamingLevelInstance>(pStreaming);
            if (!pInstanceStreaming)
              return false;

            return pInstanceStreaming->GetLevelInstanceActor() == SubLevel;
          });

  return ppStreaming ? *ppStreaming : nullptr;
}
