// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CesiumSubLevel.generated.h"

class ULevelStreaming;

/*
 * Sublevels can be georeferenced to the globe by filling out this struct.
 */
USTRUCT()
struct FCesiumSubLevel {
  GENERATED_BODY()

  /**
   * The plain name of the sub level, without any prefixes.
   */
  UPROPERTY(VisibleAnywhere, Category = "Cesium")
  FString LevelName;

  /**
   * Whether this sub-level is enabled. An enabled sub-level will be
   * automatically loaded when the camera moves within its LoadRadius and
   * no other levels are closer, and the Georeference will be updated so that
   * this level's Longitude, Latitude, and Height bcome (0, 0, 0) in the Unreal
   * World. A sub-level that is not enabled will be ignored by Cesium.
   *
   * If this option is greyed out, check that World Composition is enabled
   * in the World Settings and that this sub-level is in a Layer that has
   * Distance-based streaming DISABLED.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      Meta = (EditCondition = "CanBeEnabled"))
  bool Enabled = true;

  /**
   * The WGS84 longitude in degrees of where this level should sit on the
   * globe, in the range [-180, 180]
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      meta = (ClampMin = -180.0, ClampMax = 180.0, EditCondition = "Enabled"))
  double LevelLongitude = 0.0;

  /**
   * The WGS84 latitude in degrees of where this level should sit on the globe,
   * in the range [-90, 90]
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      meta = (ClampMin = -90.0, ClampMax = 90.0, EditCondition = "Enabled"))
  double LevelLatitude = 0.0;

  /**
   * The height in meters above the WGS84 globe this level should sit.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      meta = (EditCondition = "Enabled"))
  double LevelHeight = 0.0;

  /**
   * How far in meters from the sublevel local origin the camera needs to be to
   * load the level.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      meta = (ClampMin = 0.0, EditCondition = "Enabled"))
  double LoadRadius = 0.0;

  UPROPERTY(VisibleDefaultsOnly, Category = "Cesium")
  bool CanBeEnabled = false;
};
