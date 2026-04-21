// Copyright 2020-2025 Bentley Systems, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

#include "Subsystems/EngineSubsystem.h"

#include "CesiumGaussianSplatDataInterface.h"
#include "CesiumGltfGaussianSplatComponent.h"
#include <CesiumAsync/Promise.h>

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
  /**
   * Gets the active UCesiumGaussianSplatSubsystem instance.
   */
  static UCesiumGaussianSplatSubsystem* Get();

  virtual void Initialize(FSubsystemCollectionBase& Collection) override;
  virtual void Deinitialize() override;

  /**
   * Registers a splat component with the splat subsystem, adding it to the list
   * of splat components to render.
   */
  void RegisterSplat(UCesiumGltfGaussianSplatComponent* Component);

  /**
   * Unregisters a splat component so that it is no longer rendered.
   */
  void UnregisterSplat(UCesiumGltfGaussianSplatComponent* Component);

  /**
   * Recomputes the bounds of the Niagara system that renders the splats. This
   * should be called when the transforms of any of the registered components
   * changes.
   */
  void RecomputeBounds();

  virtual void Tick(float DeltaTime) override;
  virtual ETickableTickType GetTickableTickType() const override;
  virtual TStatId GetStatId() const override;
  virtual bool IsTickableWhenPaused() const override;
  virtual bool IsTickableInEditor() const override;
  virtual bool IsTickable() const override;

  /**
   * The currently registered splat components.
   */
  TArray<UCesiumGltfGaussianSplatComponent*> SplatComponents;

private:
  void reset();
  void initializeForWorld(UWorld& InWorld);
  void makeInterfaceDirty(bool tilesOnly = false);

  UCesiumGaussianSplatDataInterface* getDataInterface() const;

  UNiagaraComponent* _pNiagaraComponent = nullptr;
  ACesiumGaussianSplatActor* _pNiagaraActor = nullptr;
  UWorld* _pLastCreatedWorld = nullptr;
  bool _isTickEnabled = false;

  int32 _splatCount = 0;
  bool _splatInterfaceDirty = true;
  TSet<UCesiumGltfGaussianSplatComponent*> _newComponents;

  static constexpr TCHAR NiagaraSystemAssetPath[] = TEXT(
      "/Script/Niagara.NiagaraSystem'/CesiumForUnreal/GaussianSplatting/GaussianSplatSystem.GaussianSplatSystem'");
};
