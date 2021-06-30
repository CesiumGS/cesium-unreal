// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"

#include "CesiumExclusionZone.generated.h"

/**
 * A region that should be excluded from a tileset.
 *
 * This is **experimental**, and may change in future releases.
 */
USTRUCT()
struct FCesiumExclusionZone {
  GENERATED_BODY()

  /**
   * The southernmost latitude, in degrees, in the range [-90, 90]
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      meta = (ClampMin = -90.0, ClampMax = 90.0))
  double South = 0.0;

  /**
   * The westernmost longitude, in degrees, in the range [-180, 180]
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      meta = (ClampMin = -180.0, ClampMax = 180.0))
  double West = 0.0;

  /**
   * The northernmost latitude, in degrees, in the range [-90, 90]
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      meta = (ClampMin = -90.0, ClampMax = 90.0))
  double North = 0.0;

  /**
   * The easternmost longitude, in degrees, in the range [-180, 180]
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      meta = (ClampMin = -180.0, ClampMax = 180.0))
  double East = 0.0;
};
