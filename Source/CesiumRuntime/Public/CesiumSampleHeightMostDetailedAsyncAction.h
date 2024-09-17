// Copyright 2020-2024 CesiumGS, Inc. and Contributors
#pragma once

#include "CesiumSampleHeightResult.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "CesiumSampleHeightMostDetailedAsyncAction.generated.h"

class ACesium3DTileset;

/**
 * The delegate used to asynchronously return sampled heights.
 * @param Result The result of the height sampling. This array has an element
 * for each input longitude/latitude/height position. The element has a
 * HeightSampled property indicating whether the height as successfully sampled
 * at that position, and a LongitudeLatitudeHeight property with the complete
 * position including sampled height.
 * @param Warnings Provides information about problems, if any, that were
 * encountered while sampling heights.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCesiumSampleHeightMostDetailedComplete,
    const TArray<FCesiumSampleHeightResult>&,
    Result,
    const TArray<FString>&,
    Warnings);

UCLASS()
class CESIUMRUNTIME_API UCesiumSampleHeightMostDetailedAsyncAction
    : public UBlueprintAsyncActionBase {
  GENERATED_BODY()

public:
  /**
   * Asynchronously samples the height of the tileset at a list of positions,
   * each expressed as a Longitude (X) and Latitude (Y) in degrees. The Height
   * (Z) provided on input is ignored unless the sampling fails at that
   * position, in which case it is passed through to the output.
   * @param Tileset The tileset from which to query heights.
   * @param LongitudeLatitudeHeightArray The array of positions at which to
   * query heights, with Longitude in the X component and Latitude in the Y
   * component.
   */
  UFUNCTION(
      BlueprintCallable,
      Category = "Cesium",
      meta = (BlueprintInternalUseOnly = true))
  static UCesiumSampleHeightMostDetailedAsyncAction* SampleHeightMostDetailed(
      ACesium3DTileset* Tileset,
      const TArray<FVector>& LongitudeLatitudeHeightArray);

  /**
   * Called when height has been sampled at all of the given positions. The
   * Result property contains an element for each input position and in the same
   * order. The Warnings property provides information about problems that were
   * encountered while sampling heights.
   */
  UPROPERTY(BlueprintAssignable)
  FCesiumSampleHeightMostDetailedComplete OnHeightsSampled;

  virtual void Activate() override;

private:
  void RaiseOnHeightsSampled(
      ACesium3DTileset*,
      const TArray<FCesiumSampleHeightResult>&,
      const TArray<FString>&);

  ACesium3DTileset* _pTileset;
  TArray<FVector> _longitudeLatitudeHeightArray;
};
