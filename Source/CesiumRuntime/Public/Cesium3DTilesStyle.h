// Copyright 2020-2026 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumCommon.h"
#include "CesiumPropertyTable.h"
#include "UObject/Interface.h"

#include "Cesium3DTilesStyle.generated.h"

USTRUCT(BlueprintType)
struct FCesium3DTilesStyle {
  GENERATED_BODY()
public:
  /**
   * Whether or not the given feature should be shown.
   */
  UPROPERTY(BlueprintReadWrite, Category = "Cesium|Styling")
  bool bShow = true;

  /**
   * The color to apply to the given feature.
   */
  UPROPERTY(BlueprintReadWrite, Category = "Cesium|Styling")
  FColor Color = FColor(255, 255, 255, 255);
};

UINTERFACE(Blueprintable, MinimalAPI)
class UCesium3DTilesStylingProvider : public UInterface {
  GENERATED_BODY()
};

class ICesium3DTilesStylingProvider {
  GENERATED_BODY()

public:
  /**
   * Evaluates the style for the given feature.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintNativeEvent,
      Category = "Cesium|Styling")
  FCesium3DTilesStyle
  EvaluateStyle(const FCesiumPropertyTable& PropertyTable, int64 FeatureId);

  virtual FCesium3DTilesStyle OnEvaluateStyle_Implementation(
      const FCesiumPropertyTable& PropertyTable,
      int64 FeatureId) {
    return FCesium3DTilesStyle();
  }
};
