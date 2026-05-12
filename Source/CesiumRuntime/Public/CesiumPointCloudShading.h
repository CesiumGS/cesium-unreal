// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"

#include "CesiumPointCloudShading.generated.h"

/**
 * Options for adjusting how point clouds are rendered using 3D Tiles.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumPointCloudShading {
  GENERATED_USTRUCT_BODY()

  /**
   * Whether or not to perform point attenuation. Attenuation controls the size
   * of the points rendered based on the geometric error of their tile.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  bool Attenuation = false;

  /**
   * The scale to be applied to the tile's geometric error before it is used
   * to compute attenuation. Larger values will result in larger points.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta = (ClampMin = 0.0))
  float GeometricErrorScale = 1.0f;

  /**
   * The maximum point attenuation in pixels. If this is zero, the
   * Cesium3DTileset's maximumScreenSpaceError will be used as the maximum point
   * attenuation.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta = (ClampMin = 0.0))
  float MaximumAttenuation = 0.0f;

  /**
   * The average base resolution for the dataset in meters. For example,
   * a base resolution of 0.05 assumes an original capture resolution of
   * 5 centimeters between neighboring points.
   *
   * This is used in place of geometric error when the tile's geometric error is
   * 0. If this value is zero, each tile with a geometric error of 0 will have
   * its geometric error approximated instead.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta = (ClampMin = 0.0))
  float BaseResolution = 0.0f;

  bool
  operator==(const FCesiumPointCloudShading& OtherPointCloudShading) const {
    return Attenuation == OtherPointCloudShading.Attenuation &&
           GeometricErrorScale == OtherPointCloudShading.GeometricErrorScale &&
           MaximumAttenuation == OtherPointCloudShading.MaximumAttenuation &&
           BaseResolution == OtherPointCloudShading.BaseResolution;
  }

  bool
  operator!=(const FCesiumPointCloudShading& OtherPointCloudShading) const {
    return !(*this == OtherPointCloudShading);
  }
};
