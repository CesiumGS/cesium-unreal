// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"

/**
 * @brief Enumeration of the supported voxel grid shapes in the
 * `3DTILES_content_voxels` extension.
 */
enum class EVoxelGridShape : uint8 {
  Invalid = 0,
  Box = 1,
  Cylinder = 2,
  Ellipsoid = 3
};
