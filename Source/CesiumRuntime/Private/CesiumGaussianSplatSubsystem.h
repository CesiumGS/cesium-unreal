// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"

#include "CesiumGaussianSplatDataInterface.h"
#include "CesiumGltfGaussianSplatComponent.h"

#include "CesiumGaussianSplatSubsystem.generated.h"

UCLASS()
class UCesiumGaussianSplatSubsystem : public UWorldSubsystem {
  GENERATED_BODY()

public:
  UCesiumGaussianSplatSubsystem();

  void RegisterSplat(UCesiumGltfGaussianSplatComponent* Component);
  void UnregisterSplat(UCesiumGltfGaussianSplatComponent* Component);
  int32 GetNumSplats() const;

  virtual void OnWorldBeginPlay(UWorld& InWorld) override;

  UPROPERTY()
  TArray<UCesiumGltfGaussianSplatComponent*> SplatComponents;

private:
  void OnTransformUpdated(
      USceneComponent* UpdatedComponent,
      EUpdateTransformFlags UpdateTransformFlag,
      ETeleportType Teleport);

  UCesiumGaussianSplatDataInterface* GetSplatInterface() const;

  TArray<FDelegateHandle> SplatDelegateHandles;

  TObjectPtr<UNiagaraSystem> NiagaraSystemAsset;

  UPROPERTY()
  UNiagaraComponent* NiagaraComponent;
};
