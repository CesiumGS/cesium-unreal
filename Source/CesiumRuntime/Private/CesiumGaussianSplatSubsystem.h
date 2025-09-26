// Copyright 2020-2025 CesiumGS, Inc. and Contributors

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
UCLASS()
class ACesiumGaussianSplatActor : public AActor {
  GENERATED_BODY()
};

UCLASS()
class UCesiumGaussianSplatSubsystem : public UEngineSubsystem,
                                      public FTickableGameObject {
  GENERATED_BODY()

public:
  // static UCesiumGaussianSplatSubsystem* Get(UWorld* InWorld);
  virtual void Initialize(FSubsystemCollectionBase& Collection) override;
  virtual void Deinitialize() override;

  void RegisterSplat(UCesiumGltfGaussianSplatComponent* Component);
  void UnregisterSplat(UCesiumGltfGaussianSplatComponent* Component);
  void RecomputeBounds();
  int32 GetNumSplats() const;

  UPROPERTY()
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

  UPROPERTY()
  UWorld* LastCreatedWorld = nullptr;

  UPROPERTY()
  UNiagaraComponent* NiagaraComponent = nullptr;

  UPROPERTY()
  ACesiumGaussianSplatActor* NiagaraActor = nullptr;

  bool bIsTickEnabled = false;
  bool bSystemNeedsReset = false;
  int32 NumSplats = 0;
};
