// Copyright 2020-2022 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumRasterOverlay.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "CesiumWebMapServiceRasterOverlay.generated.h"

/**
 * A raster overlay that directly accesses a Web Map Service (WMS) server.
 * https://www.ogc.org/standards/wms
 */
UCLASS(ClassGroup = (Cesium), meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumWebMapServiceRasterOverlay
    : public UCesiumRasterOverlay {
  GENERATED_BODY()

public:
  /**
   * The name of the overlay
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FString OverlayName;
  /**
   * The base url of the Tile Map Service (WMS).
   * e.g.
   * https://services.ga.gov.au/gis/services/NM_Culture_and_Infrastructure/MapServer/WMSServer
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FString BaseUrl;

  /**
   * Comma separated layer names.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FString Layers;

  /**
   * True to directly specify minum and maximum zoom levels available from the
   * server, or false to automatically determine the minimum and maximum zoom
   * levels from the server's tilemapresource.xml file.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  bool bSpecifyZoomLevels = false;

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
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta = (EditCondition = "bSpecifyZoomLevels", ClampMin = 0))
  int32 MinimumLevel = 0;

  /**
   * Maximum zoom level.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta = (EditCondition = "bSpecifyZoomLevels", ClampMin = 0))
  int32 MaximumLevel = 10;

#if WITH_EDITOR

  virtual void
  PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

#endif

protected:
  virtual std::unique_ptr<Cesium3DTilesSelection::RasterOverlay> CreateOverlay(
      const Cesium3DTilesSelection::RasterOverlayOptions& options = {})
      override;
};
