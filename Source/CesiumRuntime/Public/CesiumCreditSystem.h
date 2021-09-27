// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Engine/Blueprint.h"
#include "GameFramework/Actor.h"
#include "UObject/Class.h"
#include "UObject/ConstructorHelpers.h"
#include <memory>

#include "CesiumCreditSystem.generated.h"

namespace Cesium3DTilesSelection {
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
  static ACesiumCreditSystem*
  GetDefaultCreditSystem(const UObject* WorldContextObject);

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

  const std::shared_ptr<Cesium3DTilesSelection::CreditSystem>&
  GetExternalCreditSystem() const {
    return _pCreditSystem;
  }

private:
  static UClass* CesiumCreditSystemBP;

  /**
   * A tag that is assigned to Credit Systems when they are created
   * as the "default" Credit System for a certain world.
   */
  static FName DEFAULT_CREDITSYSTEM_TAG;

  // the underlying cesium-native credit system that is managed by this actor.
  std::shared_ptr<Cesium3DTilesSelection::CreditSystem> _pCreditSystem;

  size_t _lastCreditsCount;
};
