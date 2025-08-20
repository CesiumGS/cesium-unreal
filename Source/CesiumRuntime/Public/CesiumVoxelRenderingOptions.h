// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"

#include "CesiumVoxelRenderingOptions.generated.h"

/**
 * Options for adjusting how voxels are rendered using 3D Tiles.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumVoxelRenderingOptions {
  GENERATED_USTRUCT_BODY()

  /**
   * Whether to enable linear interpolation when rendering voxels. This can
   * result in a smoother appearance across a large voxel tileset. If false,
   * nearest sampling will be used.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  bool EnableLinearInterpolation = false;

  bool operator==(
      const FCesiumVoxelRenderingOptions& OtherVoxelRenderingOptions) const {
    return EnableLinearInterpolation ==
           OtherVoxelRenderingOptions.EnableLinearInterpolation;
  }

  bool operator!=(
      const FCesiumVoxelRenderingOptions& OtherVoxelRenderingOptions) const {
    return !(*this == OtherVoxelRenderingOptions);
  }
};
