// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumRasterOverlay.h"
#include "CoreMinimal.h"
#include "CesiumGoogleMapTilesRasterOverlay.generated.h"

/**
 * A raster overlay that directly accesses Google Map Tiles (2D). If you're
 * using Google Map Tiles via Cesium ion, use the "Cesium ion Raster Overlay"
 * component instead.
 */
UCLASS(ClassGroup = Cesium, meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumGoogleMapTilesRasterOverlay
    : public UCesiumRasterOverlay {
  GENERATED_BODY()

public:
  /**
   * The Google Map Tiles API key to use.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FString Key;

protected:
  virtual std::unique_ptr<CesiumRasterOverlays::RasterOverlay> CreateOverlay(
      const CesiumRasterOverlays::RasterOverlayOptions& options = {}) override;
};
