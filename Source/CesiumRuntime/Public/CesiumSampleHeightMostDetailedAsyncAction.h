// Copyright 2020-2024 CesiumGS, Inc. and Contributors
#pragma once

#include "Kismet/BlueprintAsyncActionBase.h"
#include "CesiumSampleHeightMostDetailedAsyncAction.generated.h"

class ACesium3DTileset;

/**
 * The result of sampling a height at a position.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FSampleHeightResult {
  GENERATED_BODY()

  /**
   * The Longitude (X) and Latitude (Y) are the same values provided on input.
   * The Height (Z) is the height sampled from the tileset if the HeightSampled
   * property is true, or the original height provided on input if HeightSampled
   * is false.
   */
  UPROPERTY(BlueprintReadWrite)
  FVector LongitudeLatitudeHeight;

  /**
   * True if the height as sampled from the tileset successfully. False if the
   * tileset doesn't have any height at that position, or if something went
   * wrong. If something went wrong, the Warnings pin of the sampling function
   * will have more information about the problem.
   */
  UPROPERTY(BlueprintReadWrite)
  bool HeightSampled;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCesiumSampleHeightMostDetailedComplete,
    const TArray<FSampleHeightResult>&,
    Result,
    const TArray<FString>&,
    Warnings);

UCLASS()
class CESIUMRUNTIME_API UCesiumSampleHeightMostDetailedAsyncAction
    : public UBlueprintAsyncActionBase {
  GENERATED_BODY()

public:
  /**
   * Samples the height of the tileset at a list of positions, each expressed as
   * a Longitude (X) and Latitude (Y) in degrees. The Height (Z) provided on
   * input is ignored unless the sampling fails at that position, in which case
   * it is passed through to the output.
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
};
