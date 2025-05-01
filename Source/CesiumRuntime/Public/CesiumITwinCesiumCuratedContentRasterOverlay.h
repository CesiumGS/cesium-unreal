// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumRasterOverlay.h"
#include "CoreMinimal.h"
#include "CesiumITwinCesiumCuratedContentRasterOverlay.generated.h"

/**
 * A raster overlay that uses an IMAGERY asset from iTwin Cesium Curated
 * Content.
 */
UCLASS(
    DisplayName = "iTwin Cesium Curated Content Raster Overlay",
    ClassGroup = (Cesium),
    meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumITwinCesiumCuratedContentRasterOverlay
    : public UCesiumRasterOverlay {
  GENERATED_BODY()

public:
  /**
   * The ID of the Cesium Curated Content asset to use.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  int64 AssetID;

  /**
   * The access token to use to access the Cesium Curated Content resource.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FString ITwinAccessToken;

protected:
  virtual std::unique_ptr<CesiumRasterOverlays::RasterOverlay> CreateOverlay(
      const CesiumRasterOverlays::RasterOverlayOptions& options = {}) override;
};
