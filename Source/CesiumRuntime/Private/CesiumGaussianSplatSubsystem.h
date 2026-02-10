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
 * A blank actor type just to signify the splat singleton actor.
 */
UCLASS(Transient)
class ACesiumGaussianSplatActor : public AActor {
  GENERATED_BODY()

public:
  int32 NumSplatsSpawned = 0;
};

UCLASS()
class UCesiumGaussianSplatSubsystem : public UEngineSubsystem,
                                      public FTickableGameObject {
  GENERATED_BODY()

public:
  virtual void Initialize(FSubsystemCollectionBase& Collection) override;
  virtual void Deinitialize() override;

  void RegisterComponent(UCesiumGltfGaussianSplatComponent* Component);
  void UnregisterComponent(UCesiumGltfGaussianSplatComponent* Component);

  /**
   * Respond to changes in the visibility of any of its component.
   */
  void OnComponentVisibilityChanged();
  void RecomputeBounds();

  int32 GetNumSplats() const;

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

  UNiagaraComponent* NiagaraComponent = nullptr;

  ACesiumGaussianSplatActor* NiagaraActor = nullptr;

  UWorld* LastCreatedWorld = nullptr;
  bool bIsTickEnabled = false;
  int32 NumSplats = 0;

  // Dirty way to avoid running too many resets too quickly.
  int32 ResetFrameCounter = 0;
  bool bNeedsReset = false;
};
