#include "CesiumSubLevelSwitcherComponent.h"
#include "CesiumSubLevelComponent.h"
#include "CesiumSubLevelInstance.h"
#include "Engine/LevelStreaming.h"
#include "Engine/World.h"
#include "LevelInstance/LevelInstanceLevelStreaming.h"
#include "LevelInstance/LevelInstanceSubsystem.h"
#include "Runtime/Launch/Resources/Version.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

UCesiumSubLevelSwitcherComponent::UCesiumSubLevelSwitcherComponent() {
  this->PrimaryComponentTick.bCanEverTick = true;
}

void UCesiumSubLevelSwitcherComponent::RegisterSubLevel(
    ALevelInstance* pSubLevel) noexcept {
  this->_sublevels.AddUnique(pSubLevel);

  // In the Editor, sub-levels other than the target must initially be hidden.
#if WITH_EDITOR
  if (GEditor && IsValid(this->GetWorld()) &&
      !this->GetWorld()->IsGameWorld()) {
    if (this->_pTarget != pSubLevel) {
      pSubLevel->SetIsTemporarilyHiddenInEditor(true);
    }
  }
#endif
}

void UCesiumSubLevelSwitcherComponent::UnregisterSubLevel(
    ALevelInstance* pSubLevel) noexcept {
  // If this sub-level is active, deactivate it.
  if (this->_pTarget == pSubLevel)
    this->SetTarget(nullptr);

  this->_sublevels.Remove(pSubLevel);
}

const TArray<TWeakObjectPtr<ALevelInstance>>&
UCesiumSubLevelSwitcherComponent::GetRegisteredSubLevels() const noexcept {
  return this->_sublevels;
}

ALevelInstance* UCesiumSubLevelSwitcherComponent::GetCurrent() const noexcept {
  return this->_pCurrent.Get();
}

ALevelInstance* UCesiumSubLevelSwitcherComponent::GetTarget() const noexcept {
  return this->_pTarget.Get();
}

void UCesiumSubLevelSwitcherComponent::SetTarget(
    ALevelInstance* pLevelInstance) noexcept {
  this->_pTarget = pLevelInstance;
}

#if WITH_EDITOR

void UCesiumSubLevelSwitcherComponent::
    NotifySubLevelIsTemporarilyHiddenInEditorChanged(
        ALevelInstance* pLevelInstance,
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

#if WITH_EDITOR
  UWorld* pWorld = this->GetWorld();
  if (!IsValid(pWorld))
    return;

  if (!this->GetWorld()->IsGameWorld()) {
    this->_updateSubLevelStateEditor();
    return;
  }
#endif

  this->_updateSubLevelStateGame();
}

void UCesiumSubLevelSwitcherComponent::_updateSubLevelStateGame() {
  if (this->_pTarget == this->_pCurrent) {
    // We already match the desired state, so there's nothing to do!
    return;
  }

  // Before we can do anything else, we must make sure that any sublevels that
  // aren't pCurrent or pTarget are unloaded. This is primarily needed because
  // ALevelInstances are loaded by default and there doesn't seem to be any way
  // to disable this.
  bool anyLevelsStillLoaded = false;
  for (int32 i = 0; i < this->_sublevels.Num(); ++i) {
    ALevelInstance* pSubLevel = this->_sublevels[i].Get();
    if (!IsValid(pSubLevel))
      continue;

    if (pSubLevel == this->_pCurrent || pSubLevel == this->_pTarget)
      continue;

    ULevelStreaming* pStreaming =
        this->_getLevelStreamingForSubLevel(pSubLevel);
    ULevelStreaming::ECurrentState state =
        IsValid(pStreaming) ? pStreaming->GetCurrentState()
                            : ULevelStreaming::ECurrentState::Unloaded;
    switch (state) {
    case ULevelStreaming::ECurrentState::Loading:
    case ULevelStreaming::ECurrentState::MakingInvisible:
    case ULevelStreaming::ECurrentState::MakingVisible:
      anyLevelsStillLoaded = true;
      break;
    case ULevelStreaming::ECurrentState::FailedToLoad:
    case ULevelStreaming::ECurrentState::LoadedNotVisible:
    case ULevelStreaming::ECurrentState::LoadedVisible:
      pSubLevel->UnloadLevelInstance();
      anyLevelsStillLoaded = true;
      break;
    case ULevelStreaming::ECurrentState::Removed:
    case ULevelStreaming::ECurrentState::Unloaded:
      break;
    }
  }

  if (anyLevelsStillLoaded)
    return;

  if (this->_pCurrent != nullptr) {
    // Work toward unloading the current level.

    ULevelStreaming* pStreaming =
        this->_getLevelStreamingForSubLevel(this->_pCurrent.Get());

    ULevelStreaming::ECurrentState state =
        ULevelStreaming::ECurrentState::Unloaded;
    if (IsValid(pStreaming)) {
      state = pStreaming->GetCurrentState();
    } else if (this->_pCurrent->GetWorldAsset().IsNull()) {
      // There is no level associated with the target at all, so mark it
      // unloaded but also deactivate it for the benefit of the Editor UI.
      this->_pCurrent->UnloadLevelInstance();
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
      this->_pCurrent->UnloadLevelInstance();
      break;
    case ULevelStreaming::ECurrentState::Removed:
    case ULevelStreaming::ECurrentState::Unloaded:
      this->_pCurrent = nullptr;
      break;
    }
  }

  if (this->_pCurrent == nullptr && this->_pTarget != nullptr) {
    // Now that the current level is unloaded, work toward loading the target
    // level.

    // At this point there's no Current sub-level, so it's safe to activate the
    // Target one even though it's not loaded yet. This way, by the time the
    // level _is_ loaded, it will be at the right location because the
    // georeference has been updated.
    UCesiumSubLevelComponent* pTargetComponent =
        this->_pTarget->FindComponentByClass<UCesiumSubLevelComponent>();
    if (pTargetComponent)
      pTargetComponent->UpdateGeoreferenceIfSubLevelIsActive();

    ULevelStreaming* pStreaming =
        this->_getLevelStreamingForSubLevel(this->_pTarget.Get());

    ULevelStreaming::ECurrentState state =
        ULevelStreaming::ECurrentState::Unloaded;
    if (IsValid(pStreaming)) {
      state = pStreaming->GetCurrentState();
    } else if (this->_pTarget.Get()->GetWorldAsset().IsNull()) {
      // There is no level associated with the target at all, so mark it failed
      // to load because this is as loaded as it will ever be.
      state = ULevelStreaming::ECurrentState::FailedToLoad;
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
      // Start loading this level
      this->_pTarget.Get()->LoadLevelInstance();
      break;
    }
  }
}

#if WITH_EDITOR

void UCesiumSubLevelSwitcherComponent::_updateSubLevelStateEditor() {
  if (this->_pTarget == this->_pCurrent) {
    // We already match the desired state, so there's nothing to do!
    return;
  }

  if (this->_pCurrent != nullptr) {
    this->_pCurrent.Get()->SetIsTemporarilyHiddenInEditor(true);
    this->_pCurrent = nullptr;
  }

  if (this->_pTarget != nullptr) {
    UCesiumSubLevelComponent* pTargetComponent =
        this->_pTarget.Get()->FindComponentByClass<UCesiumSubLevelComponent>();
    if (pTargetComponent)
      pTargetComponent->UpdateGeoreferenceIfSubLevelIsActive();
    this->_pTarget.Get()->SetIsTemporarilyHiddenInEditor(false);
    this->_pCurrent = this->_pTarget;
  }
}

#endif

ULevelStreaming*
UCesiumSubLevelSwitcherComponent::_getLevelStreamingForSubLevel(
    ALevelInstance* SubLevel) const {
  if (!IsValid(SubLevel))
    return nullptr;

  ULevelStreaming* const* ppStreaming =
      GetWorld()->GetStreamingLevels().FindByPredicate(
          [SubLevel](ULevelStreaming* pStreaming) {
            ULevelStreamingLevelInstance* pInstanceStreaming =
                Cast<ULevelStreamingLevelInstance>(pStreaming);
            if (!pInstanceStreaming)
              return false;

#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION > 0
            return pInstanceStreaming->GetLevelInstance() == SubLevel;
#else
            return pInstanceStreaming->GetLevelInstanceActor() == SubLevel;
#endif
          });

  return ppStreaming ? *ppStreaming : nullptr;
}
