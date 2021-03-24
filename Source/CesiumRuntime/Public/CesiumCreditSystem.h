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

/**
 * Manages credits / atttribution for Cesium data sources. These credits
 * are displayed by the corresponding Blueprints class
 * /CesiumForUnreal/CesiumCreditSystemBP.CesiumCreditSystemBP_C.
 */
UCLASS()
class CESIUMRUNTIME_API ACesiumCreditSystem : public AActor {
  GENERATED_BODY()

public:
  static ACesiumCreditSystem* GetDefaultForActor(AActor* Actor);

  ACesiumCreditSystem();

  /**
   * The credits text to display.
   */
  UPROPERTY(BlueprintReadOnly, Category = "Cesium")
  FString Credits = "";

  /**
   * Whether the credit string has changed since last frame.
   */
  UPROPERTY(BlueprintReadOnly, Category = "Cesium")
  bool CreditsUpdated = false;

  // Called every frame
  virtual bool ShouldTickIfViewportsOnly() const override;
  virtual void Tick(float DeltaTime) override;

  const std::shared_ptr<Cesium3DTiles::CreditSystem>&
  GetExternalCreditSystem() const {
    return _pCreditSystem;
  }

private:
  static UClass* CesiumCreditSystemBP;

  // the underlying cesium-native credit system that is managed by this actor.
  std::shared_ptr<Cesium3DTiles::CreditSystem> _pCreditSystem;

  size_t _lastCreditsCount;
};
