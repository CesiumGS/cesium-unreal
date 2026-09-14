// Copyright 2020-2026 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumCommon.h"
#include "CesiumPropertyTable.h"

#include "Cesium3DTilesStyle.generated.h"

USTRUCT(BlueprintType)
struct FCesium3DTilesStyle {
  GENERATED_BODY()
public:
  UPROPERTY(BlueprintReadWrite, Category = "Cesium|Styling")
  /**
   * Whether or not the given feature should be shown.
   */
  bool bShow = true;

  UPROPERTY(BlueprintReadWrite, Category = "Cesium|Styling")
  /**
   * The color to use to highlight the given feature. This color is applied
   * multiplicatively; a color of white will result in the same appearance.
   */
  FColor Color = FColor(255, 255, 255, 255);
};

UINTERFACE(Blueprintable, MinimalAPI)
class UCesium3DTilesStylingCallbacks : public UInterface {
  GENERATED_BODY()
};

class ICesium3DTilesStylingCallbacks {
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
