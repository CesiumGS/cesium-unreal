// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Engine/Blueprint.h"
#include "GameFramework/Actor.h"
#include "UObject/Class.h"
#include "UObject/ConstructorHelpers.h"
#include <memory>

#include "CesiumCreditSystem.generated.h"

namespace Cesium3DTiles {
class CreditSystem;
}

UCLASS()
class UCesiumCreditSystemBPLoader : public UObject {
  GENERATED_BODY()

private:
  friend ACesiumCreditSystem;

  UCesiumCreditSystemBPLoader();
};

UCLASS()
class CESIUMRUNTIME_API ACesiumCreditSystem : public AActor {
  GENERATED_BODY()

public:
  static ACesiumCreditSystem* GetDefaultForActor(AActor* Actor);

  ACesiumCreditSystem();
  virtual ~ACesiumCreditSystem();

  /**
   * Credits text
   *
   */
  UPROPERTY(BlueprintReadOnly)
  FString Credits = "";

  /**
   * Whether the credit string has changed since last frame.
   *
   */
  UPROPERTY(BlueprintReadOnly)
  bool CreditsUpdated = false;

  // Called every frame
  virtual bool ShouldTickIfViewportsOnly() const override;
  virtual void Tick(float DeltaTime) override;

  const std::shared_ptr<Cesium3DTiles::CreditSystem>&
  GetExternalCreditSystem() const {
    return _pCreditSystem;
  }

protected:
  virtual void OnConstruction(const FTransform& Transform) override;

private:
  friend UCesiumCreditSystemBPLoader;

  static UClass* CesiumCreditSystemBP;

  // the underlying cesium-native credit system that is managed by this actor.
  std::shared_ptr<Cesium3DTiles::CreditSystem> _pCreditSystem;

  size_t _lastCreditsCount = 0;
};
