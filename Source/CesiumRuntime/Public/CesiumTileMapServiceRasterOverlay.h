// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumRasterOverlay.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "CesiumTileMapServiceRasterOverlay.generated.h"

/**
 * A raster overlay that directly accesses a Tile Map Service (TMS) server. If
 * you're using a Tile Map Service via Cesium ion, use the "Cesium ion Raster Overlay"
 * component instead.
 */
UCLASS(ClassGroup = (Cesium), meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumTileMapServiceRasterOverlay
    : public UCesiumRasterOverlay {
  GENERATED_BODY()

public:
  /**
   * The base URL of the Tile Map Service (TMS).
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  FString Url;

  /**
   * True to directly specify minum and maximum zoom levels available from the
   * server, or false to automatically determine the minimum and maximum zoom
   * levels from the server's tilemapresource.xml file.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  bool bSpecifyZoomLevels = false;

  /**
   * Minimum zoom level.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      meta = (EditCondition = "bSpecifyZoomLevels", ClampMin = 0))
  int32 MinimumLevel = 0;

  /**
   * Maximum zoom level.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      meta = (EditCondition = "bSpecifyZoomLevels", ClampMin = 0))
  int32 MaximumLevel = 10;

protected:
  virtual std::unique_ptr<Cesium3DTilesSelection::RasterOverlay>
  CreateOverlay() override;
};
