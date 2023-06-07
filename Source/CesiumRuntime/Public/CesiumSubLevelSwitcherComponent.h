// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#pragma once

#include "Components/ActorComponent.h"
#include "CesiumSubLevelSwitcherComponent.generated.h"

class ACesiumGeoreference;
class ACesiumSubLevelInstance;
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
  void RegisterSubLevel(ACesiumSubLevelInstance* pSubLevel) noexcept;

  /**
   * Unregisters a sub-level from this switcher. This is primarily used if the
   * sub-level is being destroyed or reparented.
   */
  void UnregisterSubLevel(ACesiumSubLevelInstance* pSubLevel) noexcept;

  /**
   * Gets the list of sub-levels that are currently registered with this
   * switcher.
   */
  const TArray<TWeakObjectPtr<ACesiumSubLevelInstance>>&
  GetRegisteredSubLevels() const noexcept;

  /**
   * Gets the sub-level that is currently active, or nullptr if none are active.
   */
  ACesiumSubLevelInstance* GetCurrent() const noexcept;

  /**
   * Gets the sub-level that is in the process of becoming active. If nullptr,
   * the target is a state where no sub-levels are active.
   */
  ACesiumSubLevelInstance* GetTarget() const noexcept;

  /**
   * Sets the sub-level that should be active. The switcher will asynchronously
   * load and show this sub-level.
   */
  void SetTarget(ACesiumSubLevelInstance* pLevelInstance) noexcept;

#if WITH_EDITOR
  void NotifySubLevelIsTemporarilyHiddenInEditorChanged(
      ACesiumSubLevelInstance* pLevelInstance,
      bool bIsHidden);
#endif

  virtual void TickComponent(
      float DeltaTime,
      enum ELevelTick TickType,
      FActorComponentTickFunction* ThisTickFunction) override;

private:
  /**
   * Ensures that there are not multiple visible sub-levels by deactivating
   * any additional sublevels after the first one.
   */
  void _ensureZeroOrOneSubLevelsAreActive();

  void _deactivateSubLevel(ACesiumSubLevelInstance* SubLevel);
  void _activateSubLevel(ACesiumSubLevelInstance* SubLevel);
  bool _isSubLevelActive(ACesiumSubLevelInstance* SubLevel) const;

  /**
   * Finds the ULevelStreaming instance, if any, associated with a given
   * sublevel.
   */
  ULevelStreaming*
  _getLevelStreamingForSubLevel(ACesiumSubLevelInstance* SubLevel) const;

  UPROPERTY(Transient, DuplicateTransient, TextExportTransient)
  TArray<TWeakObjectPtr<ACesiumSubLevelInstance>> _sublevels;

  UPROPERTY(Transient, DuplicateTransient, TextExportTransient)
  ACesiumSubLevelInstance* _pCurrent = nullptr;

  UPROPERTY(Transient, DuplicateTransient, TextExportTransient)
  ACesiumSubLevelInstance* _pTarget = nullptr;
};
