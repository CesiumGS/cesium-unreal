#include "CesiumSubLevelSwitcherComponent.h"
#include "CesiumRuntime.h"
#include "CesiumSubLevelComponent.h"
#include "Engine/LevelStreaming.h"
#include "Engine/World.h"
#include "LevelInstance/LevelInstanceActor.h"
#include "LevelInstance/LevelInstanceLevelStreaming.h"
#include "LevelInstance/LevelInstanceSubsystem.h"
#include "Runtime/Launch/Resources/Version.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

namespace {

FString GetActorLabel(AActor* pActor) {
  if (!IsValid(pActor))
    return TEXT("<none>");

#if WITH_EDITOR
  return pActor->GetActorLabel();
#else
  return pActor->GetName();
#endif
}

} // namespace

UCesiumSubLevelSwitcherComponent::UCesiumSubLevelSwitcherComponent() {
  this->PrimaryComponentTick.bCanEverTick = true;
}

void UCesiumSubLevelSwitcherComponent::RegisterSubLevel(
    ALevelInstance* pSubLevel) noexcept {
  this->_sublevels.AddUnique(pSubLevel);

  // Do extra checks on the next tick so that if we're in a game and this level
  // is already loaded and shouldn't be, we can unload it.
  this->_doExtraChecksOnNextTick = true;

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
  this->_sublevels.Remove(pSubLevel);

  // Next tick, we need to check if the target is still registered, in case this
  // method call just removed it. But we can't actually do the check here
  // because the Editor UI goes through an unregister/re-register cycle
  // _constantly_, and we don't want to forget the target sub-level just because
  // it was edited in the UI.
  this->_doExtraChecksOnNextTick = true;
}

TArray<ALevelInstance*>
UCesiumSubLevelSwitcherComponent::GetRegisteredSubLevels() const noexcept {
  TArray<ALevelInstance*> result;
  result.Reserve(this->_sublevels.Num());
  for (const TWeakObjectPtr<ALevelInstance>& pWeak : this->_sublevels) {
    ALevelInstance* p = pWeak.Get();
    if (p)
      result.Add(p);
  }
  return result;
}

ALevelInstance*
UCesiumSubLevelSwitcherComponent::GetCurrentSubLevel() const noexcept {
  return this->_pCurrent.Get();
}

ALevelInstance*
UCesiumSubLevelSwitcherComponent::GetTargetSubLevel() const noexcept {
  return this->_pTarget.Get();
}

void UCesiumSubLevelSwitcherComponent::SetTargetSubLevel(
    ALevelInstance* pLevelInstance) noexcept {
  if (this->_pTarget != pLevelInstance) {
    if (pLevelInstance) {
      UE_LOG(
          LogCesium,
          Display,
          TEXT("New target sub-level %s."),
          *GetActorLabel(pLevelInstance));
    } else {
      UE_LOG(LogCesium, Display, TEXT("New target sub-level <none>"));
    }

    this->_pTarget = pLevelInstance;
    this->_isTransitioningSubLevels = true;
  }
}

#ifdef ENGINE_VERSION_5_3_OR_HIGHER
#define StreamState ELevelStreamingState
#else
#define StreamState ULevelStreaming::ECurrentState
#endif

void UCesiumSubLevelSwitcherComponent::TickComponent(
    float DeltaTime,
    enum ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction) {
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  if (this->_doExtraChecksOnNextTick) {
    if (this->_pTarget != nullptr &&
        this->_sublevels.Find(this->_pTarget) == INDEX_NONE) {
      // Target level is no longer registered, so the new target is "none".
      this->SetTargetSubLevel(nullptr);
    }

    // In game, make sure that any sub-levels that aren't pCurrent or pTarget
    // are unloaded. This is primarily needed because ALevelInstances are loaded
    // by default and there doesn't seem to be any way to disable this. In the
    // Editor, levels pretty much stay loaded all the time.
    if (this->GetWorld()->IsGameWorld()) {
      bool anyLevelsStillLoaded = false;
      for (int32 i = 0; i < this->_sublevels.Num(); ++i) {
        ALevelInstance* pSubLevel = this->_sublevels[i].Get();
        if (!IsValid(pSubLevel))
          continue;

        if (pSubLevel == this->_pCurrent || pSubLevel == this->_pTarget)
          continue;


        ULevelStreaming* pStreaming =
            this->_getLevelStreamingForSubLevel(pSubLevel);
        StreamState state =
            IsValid(pStreaming)
#ifdef ENGINE_VERSION_5_3_OR_HIGHER
                ? pStreaming->GetLevelStreamingState() //->GetCurrentState()
#else
                ? pStreaming->GetCurrentState()
#endif
                : StreamState::Unloaded;

        switch (state) {
        case StreamState::Loading:
        case StreamState::MakingInvisible:
        case StreamState::MakingVisible:
          anyLevelsStillLoaded = true;
          break;
        case StreamState::FailedToLoad:
        case StreamState::LoadedNotVisible:
        case StreamState::LoadedVisible:
          pSubLevel->UnloadLevelInstance();
          anyLevelsStillLoaded = true;
          break;
        case StreamState::Removed:
        case StreamState::Unloaded:
          break;
        }
      }

      if (anyLevelsStillLoaded) {
        // Don't do anything else until the levels are unloaded.
        return;
      }
    }

    this->_doExtraChecksOnNextTick = false;
  }

#ifdef WITH_EDITOR
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
  if (this->_isTransitioningSubLevels && this->_pCurrent == this->_pTarget) {
    // It's possible that the pCurrent sub-level was active, then we briefly set
    // pTarget to something else to trigger an unload of pCurrent, and then
    // immediately set pTarget back to pCurrent. So we detect that here.
    this->_pCurrent = nullptr;
  }

  if (this->_pCurrent == this->_pTarget) {
    // We already match the desired state, so there's nothing to do!
    return;
  }

  this->_isTransitioningSubLevels = false;

  if (this->_pCurrent != nullptr) {
    // Work toward unloading the current level.

    ULevelStreaming* pStreaming =
        this->_getLevelStreamingForSubLevel(this->_pCurrent.Get());

    StreamState state = StreamState::Unloaded;
    if (IsValid(pStreaming)) {
#ifdef ENGINE_VERSION_5_3_OR_HIGHER
      state = pStreaming->GetLevelStreamingState();
      #else
      state = pStreaming->GetCurrentState();
      #endif
      
    } else if (this->_pCurrent->GetWorldAsset().IsNull()) {
      // There is no level associated with the target at all, so mark it
      // unloaded but also deactivate it for the benefit of the Editor UI.
      this->_pCurrent->UnloadLevelInstance();
    }

    switch (state) {
    case StreamState::Loading:
    case StreamState::MakingInvisible:
    case StreamState::MakingVisible:
      // Wait for these transitions to finish before doing anything further.
      // TODO: maybe we can cancel these transitions somehow?
      UE_LOG(
          LogCesium,
          Log,
          TEXT(
              "Waiting for sub-level %s to transition out of an intermediate state while unloading it."),
          *GetActorLabel(this->_pCurrent.Get()));
      this->_isTransitioningSubLevels = true;
      break;
    case StreamState::FailedToLoad:
    case StreamState::LoadedNotVisible:
    case StreamState::LoadedVisible:
      UE_LOG(
          LogCesium,
          Display,
          TEXT("Starting unload of sub-level %s."),
          *GetActorLabel(this->_pCurrent.Get()));
      this->_isTransitioningSubLevels = true;
      this->_pCurrent->UnloadLevelInstance();
      break;
    case StreamState::Removed:
    case StreamState::Unloaded:
      UE_LOG(
          LogCesium,
          Display,
          TEXT("Finished unloading sub-level %s."),
          *GetActorLabel(this->_pCurrent.Get()));
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

    StreamState state = StreamState::Unloaded;
    if (IsValid(pStreaming)) {
#ifdef ENGINE_VERSION_5_3_OR_HIGHER 
      state = pStreaming->GetLevelStreamingState();
      #else

      state = pStreaming->GetCurrentState();
      #endif
    } else if (this->_pTarget.Get()->GetWorldAsset().IsNull()) {
      // There is no level associated with the target at all, so mark it failed
      // to load because this is as loaded as it will ever be.
      state = StreamState::FailedToLoad;
    }

    switch (state) {
    case StreamState::Loading:
    case StreamState::MakingInvisible:
    case StreamState::MakingVisible:
      // Wait for these transitions to finish before doing anything further.
      UE_LOG(
          LogCesium,
          Log,
          TEXT(
              "Waiting for sub-level %s to transition out of an intermediate state while loading it."),
          *GetActorLabel(this->_pTarget.Get()));
      this->_isTransitioningSubLevels = true;
      break;
    case StreamState::FailedToLoad:
    case StreamState::LoadedNotVisible:
    case StreamState::LoadedVisible:
      // Loading complete!
      UE_LOG(
          LogCesium,
          Display,
          TEXT("Finished loading sub-level %s."),
          *GetActorLabel(this->_pTarget.Get()));

      // Double-check that we're not actively trying to unload this level
      // already. If we are, wait longer.
      if ((IsValid(pStreaming) && pStreaming->ShouldBeLoaded()) ||
          this->_pTarget.Get()->GetWorldAsset().IsNull()) {
        this->_pCurrent = this->_pTarget;
      } else {
        this->_isTransitioningSubLevels = true;
      }
      break;
    case StreamState::Removed:
    case StreamState::Unloaded:
      // Start loading this level
      UE_LOG(
          LogCesium,
          Display,
          TEXT("Starting load of sub-level %s."),
          *GetActorLabel(this->_pTarget.Get()));
      this->_isTransitioningSubLevels = true;
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

            return pInstanceStreaming->GetLevelInstance() == SubLevel;
          });

  return ppStreaming ? *ppStreaming : nullptr;
}
