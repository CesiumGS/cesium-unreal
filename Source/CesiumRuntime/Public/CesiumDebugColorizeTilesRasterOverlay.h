// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumRasterOverlay.h"
#include "CoreMinimal.h"
#include "CesiumDebugColorizeTilesRasterOverlay.generated.h"

/**
 * A raster overlay that can be used to debug tilesets by shading each tile with
 * a random color.
 */
UCLASS(
    DisplayName = "Cesium Debug Colorize Tiles Raster Overlay",
    ClassGroup = (Cesium),
    meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumDebugColorizeTilesRasterOverlay
    : public UCesiumRasterOverlay {
  GENERATED_BODY()

public:
protected:
  virtual std::unique_ptr<Cesium3DTilesSelection::RasterOverlay> CreateOverlay(
      const Cesium3DTilesSelection::RasterOverlayOptions& options = {})
      override;
};
