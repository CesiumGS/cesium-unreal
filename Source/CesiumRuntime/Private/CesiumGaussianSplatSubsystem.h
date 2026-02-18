// Copyright 2020-2025 Bentley Systems, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

#include "Subsystems/EngineSubsystem.h"

#include "CesiumGaussianSplatDataInterface.h"
#include "CesiumGltfGaussianSplatComponent.h"

#include "CesiumGaussianSplatSubsystem.generated.h"

/**
 * A singleton actor attached to the Niagara system spawned to render gaussian
 * splats, allowing us to ensure that only one system exists in a UWorld at any
 * given time.
 */
UCLASS(Transient)
class ACesiumGaussianSplatActor : public AActor {
  GENERATED_BODY()
};

UCLASS()
class UCesiumGaussianSplatSubsystem : public UEngineSubsystem,
                                      public FTickableGameObject {
  GENERATED_BODY()

public:
  virtual void Initialize(FSubsystemCollectionBase& Collection) override;
  virtual void Deinitialize() override;

  /**
   * Registers a splat component with the splat subsystem, adding it to the list
   * of splat components to render.
   */
  void RegisterSplat(UCesiumGltfGaussianSplatComponent* Component);
  /**
   * Unregisters a previously-registered splat component, removing it from the
   * list of splat components to render.
   */
  void UnregisterSplat(UCesiumGltfGaussianSplatComponent* Component);
  /**
   * Recomputes the bounds of the Niagara system that renders the splats. This
   * should be called when the transforms of any of the registered components
   * changes.
   */
  void RecomputeBounds();

  /**
   * The currently-registerd splat components.
   */
  TArray<UCesiumGltfGaussianSplatComponent*> SplatComponents;

  virtual void Tick(float DeltaTime) override;
  virtual ETickableTickType GetTickableTickType() const override;
  virtual TStatId GetStatId() const override;
  virtual bool IsTickableWhenPaused() const override;
  virtual bool IsTickableInEditor() const override;
  virtual bool IsTickable() const override;

private:
  void InitializeForWorld(UWorld& InWorld);

  void UpdateNiagaraComponent();
  UCesiumGaussianSplatDataInterface* GetSplatInterface() const;

  UNiagaraComponent* _pNiagaraComponent = nullptr;

  ACesiumGaussianSplatActor* _pNiagaraActor = nullptr;

  UWorld* _pLastCreatedWorld = nullptr;
  bool _isTickEnabled = false;
  int32 _numSplats = 0;

  // Dirty way to avoid running too many resets too quickly.
  int32 _resetFrameCounter = 0;
  bool _needsReset = false;
};
