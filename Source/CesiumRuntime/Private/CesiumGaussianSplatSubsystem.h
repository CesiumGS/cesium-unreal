// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#pragma once

#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Subsystems/WorldSubsystem.h"

#include "CesiumGaussianSplatDataInterface.h"
#include "CesiumGltfGaussianSplatComponent.h"

#include "CesiumGaussianSplatSubsystem.generated.h"

UCLASS()
class UCesiumGaussianSplatSubsystem : public UWorldSubsystem {
  GENERATED_BODY()

public:
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

  UPROPERTY()
  UNiagaraComponent* NiagaraComponent;

  UPROPERTY()
  AActor* NiagaraActor;
};
