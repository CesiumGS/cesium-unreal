// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumRasterOverlay.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "CesiumWebMapServiceRasterOverlay.generated.h"

/**
 * A raster overlay that directly accesses a Web Map Service (WMS) server.
 * https://www.ogc.org/standards/wms
 */
UCLASS(ClassGroup = Cesium, meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumWebMapServiceRasterOverlay
    : public UCesiumRasterOverlay {
  GENERATED_BODY()

public:
  /**
   * The base url of the Web Map Service (WMS).
   * e.g.
   * https://services.ga.gov.au/gis/services/NM_Culture_and_Infrastructure/MapServer/WMSServer
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FString BaseUrl;

  /**
   * Comma-separated layer names to request from the server.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FString Layers;

  /**
   * Image width
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta = (ClampMin = 64, ClampMax = 2048))
  int32 TileWidth = 256;

  /**
   * Image height
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta = (ClampMin = 64, ClampMax = 2048))
  int32 TileHeight = 256;

  /**
   * Minimum zoom level.
   *
   * Take care when specifying this that the number of tiles at the minimum
   * level is small, such as four or less. A larger number is likely to
   * result in rendering problems.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta = (ClampMin = 0))
  int32 MinimumLevel = 0;

  /**
   * Maximum zoom level.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta = (ClampMin = 0))
  int32 MaximumLevel = 14;

  /**
   * HTTP headers to be attached to each request made for this raster overlay.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  TMap<FString, FString> RequestHeaders;

protected:
  virtual std::unique_ptr<CesiumRasterOverlays::RasterOverlay> CreateOverlay(
      const CesiumRasterOverlays::RasterOverlayOptions& options = {}) override;
};
