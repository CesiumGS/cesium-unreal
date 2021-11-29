// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumRasterOverlay.h"
#include "CoreMinimal.h"
#include "CesiumIonRasterOverlay.generated.h"

/**
 * A raster overlay that uses an IMAGERY asset from Cesium ion.
 */
UCLASS(
    DisplayName = "Cesium ion Raster Overlay",
    ClassGroup = (Cesium),
    meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumIonRasterOverlay : public UCesiumRasterOverlay {
  GENERATED_BODY()

public:
  /**
   * The ID of the Cesium ion asset to use.
   *
   * If this property is non-zero, the Bing Maps Key and Map Style properties
   * are ignored.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  int64 IonAssetID;

  /**
   * The access token to use to access the Cesium ion resource.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FString IonAccessToken;

protected:
  virtual std::unique_ptr<Cesium3DTilesSelection::RasterOverlay> CreateOverlay(
      const Cesium3DTilesSelection::RasterOverlayOptions& options = {})
      override;
};
