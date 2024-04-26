// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumRasterOverlay.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "CesiumWebMapTileServiceRasterOverlay.generated.h"

/**
 * A raster overlay that directly accesses a Web Map Tile Service (WMTS) server.
 * If you're using a Web Map Tile Service via Cesium ion, use the "Cesium ion
 * Raster Overlay" component instead.
 */
UCLASS(ClassGroup = (Cesium), meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumWebMapTileServiceRasterOverlay
    : public UCesiumRasterOverlay {
  GENERATED_BODY()

public:
  /**
   * The base URL of the Web Map Tile Service (WMTS).
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FString Url;

  /**
   * Layer name.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FString Layer;

  /**
   * Style.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FString Style;

  /**
   * Format.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FString Format = "image/jpeg";

  /**
   * Tile Matrix Set ID.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FString TileMatrixSetID;

  /**
   * True to specify tile matrix set labels manually, or false to
   * automatically determine from level and prefix.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  bool bSpecifyTileMatrixSetLabels = false;

  /**
   * Prefix for tile matrix set labels. For instance, setting "EPSG:4326:" as
   * prefix generates label list ["EPSG:4326:0", "EPSG:4326:1", "EPSG:4326:2",
   * ...]
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta = (EditCondition = "!bSpecifyTileMatrixSetLabels"))
  FString TileMatrixSetLabelPrefix;

  /**
   * Tile Matrix Set Labels.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta = (EditCondition = "bSpecifyTileMatrixSetLabels"))
  TArray<FString> TileMatrixSetLabels;

  /**
   * False to use geographic projection, true to use webmercator projection.
   * For instance, EPSG:4326 uses geographic and EPSG:3857 uses webmercator.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  bool UseWebMercatorProjection;

  /**
   * True to specify quadtree tiling scheme according to projection and
   * bounding rectangle, or false to automatically determine from projection.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  bool bSpecifyTilingScheme = false;

  /**
   * Tile number corresponding to TileCol, also known as TileMatrixWidth.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta = (EditCondition = "bSpecifyTilingScheme", ClampMin = 1))
  int32 RootTilesX = 1;

  /**
   * Tile number corresponding to TileRow, also known as TileMatrixHeight.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta = (EditCondition = "bSpecifyTilingScheme", ClampMin = 1))
  int32 RootTilesY = 1;

  /**
   * The longitude of the west boundary on globe in degrees, in the range
   * [-180, 180]
   */
  UPROPERTY(
      Category = "Cesium",
      EditAnywhere,
      BlueprintReadWrite,
      meta =
          (EditCondition = "bSpecifyTilingScheme",
           ClampMin = -180.0,
           ClampMax = 180.0))
  double West = -180;

  /**
   * The latitude of the south boundary on globe in degrees, in the range
   * [-90, 90]
   */
  UPROPERTY(
      Category = "Cesium",
      EditAnywhere,
      BlueprintReadWrite,
      meta =
          (EditCondition = "bSpecifyTilingScheme",
           ClampMin = -90.0,
           ClampMax = 90.0))
  double South = -90;

  /**
   * The longitude of the west boundary in degrees, in the range [-180, 180]
   */
  UPROPERTY(
      Category = "Cesium",
      EditAnywhere,
      BlueprintReadWrite,
      meta =
          (EditCondition = "bSpecifyTilingScheme",
           ClampMin = -180.0,
           ClampMax = 180.0))
  double East = 180;

  /**
   * The latitude of the south boundary in degrees, in the range [-90, 90]
   */
  UPROPERTY(
      Category = "Cesium",
      EditAnywhere,
      BlueprintReadWrite,
      meta =
          (EditCondition = "bSpecifyTilingScheme",
           ClampMin = -90.0,
           ClampMax = 90.0))
  double North = 90;

  /**
   * True to directly specify minum and maximum zoom levels available from the
   * server, or false to automatically determine the minimum and maximum zoom
   * levels from the server's tilemapresource.xml file.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  bool bSpecifyZoomLevels = false;

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
  int32 MaximumLevel = 25;

protected:
  virtual std::unique_ptr<CesiumRasterOverlays::RasterOverlay> CreateOverlay(
      const CesiumRasterOverlays::RasterOverlayOptions& options = {}) override;
};
