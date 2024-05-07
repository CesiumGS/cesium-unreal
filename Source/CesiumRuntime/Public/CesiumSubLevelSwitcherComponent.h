// Copyright 2020-2024 CesiumGS, Inc. and Contributors

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
UCLASS(ClassGroup = "Cesium")
class CESIUMRUNTIME_API UCesiumSubLevelSwitcherComponent
    : public UActorComponent {
  GENERATED_BODY()

public:
  UCesiumSubLevelSwitcherComponent();

  /**
   * Gets the list of sub-levels that are currently registered with this
   * switcher.
   */
  UFUNCTION(BlueprintPure, Category = "Cesium|Sub-levels")
  TArray<ALevelInstance*> GetRegisteredSubLevels() const noexcept;

  /**
   * Gets the list of sub-levels that are currently registered with this
   * switcher. This is slightly more efficient than GetRegisteredSubLevels but
   * is not accessible from Blueprints.
   */
  const TArray<TWeakObjectPtr<ALevelInstance>>&
  GetRegisteredSubLevelsWeak() const noexcept {
    return this->_sublevels;
  }

  /**
   * Gets the sub-level that is currently active, or nullptr if none are active.
   */
  UFUNCTION(BlueprintPure, Category = "Cesium|Sub-levels")
  ALevelInstance* GetCurrentSubLevel() const noexcept;

  /**
   * Gets the sub-level that is in the process of becoming active. If nullptr,
   * the target is a state where no sub-levels are active.
   */
  UFUNCTION(BlueprintPure, Category = "Cesium|Sub-levels")
  ALevelInstance* GetTargetSubLevel() const noexcept;

  /**
   * Sets the sub-level that should be active. The switcher will asynchronously
   * load and show this sub-level.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium|Sub-levels")
  void SetTargetSubLevel(ALevelInstance* LevelInstance) noexcept;

private:
  // To allow the sub-level to register/unregister itself with the functions
  // below.
  friend class UCesiumSubLevelComponent;
  friend class CesiumEditorSubLevelMutex;

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

  virtual void TickComponent(
      float DeltaTime,
      enum ELevelTick TickType,
      FActorComponentTickFunction* ThisTickFunction) override;

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
};
