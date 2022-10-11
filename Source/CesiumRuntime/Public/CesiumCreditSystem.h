// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Components/WidgetComponent.h"
#include "Engine/Blueprint.h"
#include "GameFramework/Actor.h"
#include "UObject/Class.h"
#include "UObject/ConstructorHelpers.h"
#include <memory>
#include <unordered_map>

#include "CesiumCreditSystem.generated.h"

namespace Cesium3DTilesSelection {
class CreditSystem;
}

/**
 * Manages credits / atttribution for Cesium data sources. These credits
 * are displayed by the corresponding Blueprints class
 * /CesiumForUnreal/CesiumCreditSystemBP.CesiumCreditSystemBP_C.
 */
UCLASS(Abstract)
class CESIUMRUNTIME_API ACesiumCreditSystem : public AActor {
  GENERATED_BODY()

public:
  static ACesiumCreditSystem*
  GetDefaultCreditSystem(const UObject* WorldContextObject);

  ACesiumCreditSystem();

  void BeginPlay() override;

  UPROPERTY(EditDefaultsOnly, Category = "Cesium")
  TSubclassOf<UUserWidget> CreditsWidgetClass;

  /**
   * Whether the credit string has changed since last frame.
   */
  UPROPERTY(BlueprintReadOnly, Category = "Cesium")
  bool CreditsUpdated = false;

  UPROPERTY(BlueprintReadOnly, Category = "Cesium")
  class UScreenCreditsWidget* CreditsWidget;

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

  FString ConvertHtmlToRtf(std::string html);
  std::unordered_map<std::string, FString> _htmlToRtf;
};
