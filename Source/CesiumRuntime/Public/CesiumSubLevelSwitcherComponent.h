// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#pragma once

#include "Components/ActorComponent.h"
#include "CesiumSubLevelSwitcherComponent.generated.h"

class ACesiumGeoreference;
class ALevelInstance;
class ULevelStreaming;
class UWorld;

/**
 * Manages the asynchronous switching between sub-levels, making sure that a
 * previous sub-level is hidden before the CesiumGeoreference is switched to a
 * new location and the next sub-level is loaded.
 */
UCLASS()
class CESIUMRUNTIME_API UCesiumSubLevelSwitcherComponent
    : public UActorComponent {
  GENERATED_BODY()

public:
  UCesiumSubLevelSwitcherComponent();

  /**
   * Registers a sub-level with this switcher. The switcher will ensure that
   * no more than one of the registered sub-levels is active at any time.
   */
  void RegisterSubLevel(ALevelInstance* pSubLevel) noexcept;

  /**
   * Unregisters a sub-level from this switcher. This is primarily used if the
   * sub-level is being destroyed or reparented.
   */
  void UnregisterSubLevel(ALevelInstance* pSubLevel) noexcept;

  /**
   * Gets the list of sub-levels that are currently registered with this
   * switcher.
   */
  const TArray<TWeakObjectPtr<ALevelInstance>>&
  GetRegisteredSubLevels() const noexcept;

  /**
   * Gets the sub-level that is currently active, or nullptr if none are active.
   */
  ALevelInstance* GetCurrent() const noexcept;

  /**
   * Gets the sub-level that is in the process of becoming active. If nullptr,
   * the target is a state where no sub-levels are active.
   */
  ALevelInstance* GetTarget() const noexcept;

  /**
   * Sets the sub-level that should be active. The switcher will asynchronously
   * load and show this sub-level.
   */
  void SetTarget(ALevelInstance* pLevelInstance) noexcept;

#if WITH_EDITOR
  void NotifySubLevelIsTemporarilyHiddenInEditorChanged(
      ALevelInstance* pLevelInstance,
      bool bIsHidden);
#endif

  virtual void TickComponent(
      float DeltaTime,
      enum ELevelTick TickType,
      FActorComponentTickFunction* ThisTickFunction) override;

private:
  void _updateSubLevelStateGame();
#if WITH_EDITOR
  void _updateSubLevelStateEditor();
#endif

  /**
   * Finds the ULevelStreaming instance, if any, associated with a given
   * sub-level.
   */
  ULevelStreaming*
  _getLevelStreamingForSubLevel(ALevelInstance* SubLevel) const;

  // Don't save/load or copy this.
  UPROPERTY(Transient, DuplicateTransient, TextExportTransient)
  TArray<TWeakObjectPtr<ALevelInstance>> _sublevels;

  // Don't save/load or copy this.
  UPROPERTY(Transient, DuplicateTransient, TextExportTransient)
  TWeakObjectPtr<ALevelInstance> _pCurrent = nullptr;

  // Save/load this, but don't copy it.
  UPROPERTY(DuplicateTransient, TextExportTransient)
  TWeakObjectPtr<ALevelInstance> _pTarget = nullptr;

  bool _doExtraChecksOnNextTick = false;
  bool _isTransitioningSubLevels = false;

  friend class CesiumEditorSubLevelMutex;
};
