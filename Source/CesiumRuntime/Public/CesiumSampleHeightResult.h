// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumSampleHeightResult.generated.h"

/**
 * The result of sampling the height on a tileset at the given cartographic
 * position.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumSampleHeightResult {
  GENERATED_BODY()

  /**
   * The Longitude (X) and Latitude (Y) are the same values provided on input.
   * The Height (Z) is the height sampled from the tileset if the HeightSampled
   * property is true, or the original height provided on input if HeightSampled
   * is false.
   */
  UPROPERTY(BlueprintReadWrite, Category = "Cesium")
  FVector LongitudeLatitudeHeight;

  /**
   * True if the height as sampled from the tileset successfully. False if the
   * tileset doesn't have any height at that position, or if something went
   * wrong. If something went wrong, the Warnings pin of the sampling function
   * will have more information about the problem.
   */
  UPROPERTY(BlueprintReadWrite, Category = "Cesium")
  bool HeightSampled;
};
