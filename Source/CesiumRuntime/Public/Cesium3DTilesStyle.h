// Copyright 2020-2026 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumCommon.h"
#include "CesiumPropertyTable.h"

#include "Cesium3DTilesStyle.generated.h"

UINTERFACE(Blueprintable, MinimalAPI)
class UCesium3DTilesStylingCallbacks : public UInterface {
  GENERATED_BODY()
};

class ICesium3DTilesStylingCallbacks {
  GENERATED_BODY()

public:
  /**
   * Evaluates whether or not the given feature should be shown.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintNativeEvent,
      Category = "Cesium|Styling")
  bool EvaluateShow(const FCesiumPropertyTable& PropertyTable, int64 FeatureId);

  virtual bool OnEvaluateShow_Implementation(
      const FCesiumPropertyTable& PropertyTable,
      int64 FeatureId) {
    return true;
  }

  /**
   * Evalutes what color to use to highlight the given feature.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintNativeEvent,
      Category = "Cesium|Styling")
  FColor
  EvaluateColor(const FCesiumPropertyTable& PropertyTable, int64 FeatureId);

  virtual FColor OnEvaluateColor_Implementation(
      const FCesiumPropertyTable& PropertyTable,
      int64 FeatureId) {
    return FColor(255, 255, 255);
  }
};
