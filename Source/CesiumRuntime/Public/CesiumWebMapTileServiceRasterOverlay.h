// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumRasterOverlay.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "CesiumWebMapTileServiceRasterOverlay.generated.h"

/**
 * Specifies the type of projection used for projecting a Web Map Tile Service
 * raster overlay.
 */
UENUM(BlueprintType)
enum class ECesiumWebMapTileServiceRasterOverlayProjection : uint8 {
  /**
   * The raster overlay is projected using Web Mercator.
   */
  WebMercator,

  /**
   * The raster overlay is projected using a geographic projection.
   */
  Geographic
};

/**
 * A raster overlay that directly accesses a Web Map Tile Service (WMTS) server.
 * If you're using a Web Map Tile Service via Cesium ion, use the "Cesium ion
 * Raster Overlay" component instead.
 */
UCLASS(ClassGroup = Cesium, meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumWebMapTileServiceRasterOverlay
    : public UCesiumRasterOverlay {
  GENERATED_BODY()

public:
  /**
   * The base URL of the Web Map Tile Service (WMTS).
   * This URL should not include query parameters. For example:
   * https://tile.openstreetmap.org/{TileMatrix}/{TileCol}/{TileRow}.png
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FString BaseUrl;

  /**
   * The layer name for WMTS requests.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FString Layer;

  /**
   * The style name for WMTS requests.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FString Style;

  /**
   * The MIME type for images to retrieve from the server.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FString Format = "image/jpeg";

  /**
   * The tile matrix set identifier for WMTS requests.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FString TileMatrixSetID;

  /**
   * The prefix to use for the tile matrix set labels. For instance, setting
   * "EPSG:4326:" as prefix generates label list ["EPSG:4326:0", "EPSG:4326:1",
   * "EPSG:4326:2", ...]
   * Only applicable when "Specify Tile Matrix Set Labels" is false.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta = (EditCondition = "!bSpecifyTileMatrixSetLabels"))
  FString TileMatrixSetLabelPrefix;

  /**
   * Set this to true to specify tile matrix set labels manually. If false, the
   * labels will be constructed from the specified levels and prefix (if one is
   * specified).
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  bool bSpecifyTileMatrixSetLabels = false;

  /**
   * The manually specified tile matrix set labels.
   *
   * Only applicable when "Specify Tile Matrix Set Labels" is true.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta = (EditCondition = "bSpecifyTileMatrixSetLabels"))
  TArray<FString> TileMatrixSetLabels;

  UPROPERTY(
      meta =
          (DeprecatedProperty, DeprecationMessage = "Use Projection instead."))
  bool UseWebMercatorProjection_DEPRECATED;

  /**
   * The type of projection used to project the WMTS imagery onto the globe.
   * For instance, EPSG:4326 uses geographic projection and EPSG:3857 uses Web
   * Mercator.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  ECesiumWebMapTileServiceRasterOverlayProjection Projection =
      ECesiumWebMapTileServiceRasterOverlayProjection::WebMercator;

  /**
   * Set this to true to specify the quadtree tiling scheme according to the
   * specified root tile numbers and projected bounding rectangle. If false, the
   * tiling scheme will be deduced from the projection.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  bool bSpecifyTilingScheme = false;

  /**
   * The number of tiles corresponding to TileCol, also known as
   * TileMatrixWidth. If specified, this determines the number of tiles at the
   * root of the quadtree tiling scheme in the X direction.
   *
   * Only applicable if "Specify Tiling Scheme" is set to true.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta = (EditCondition = "bSpecifyTilingScheme", ClampMin = 1))
  int32 RootTilesX = 1;

  /**
   * The number of tiles corresponding to TileRow, also known as
   * TileMatrixHeight. If specified, this determines the number of tiles at the
   * root of the quadtree tiling scheme in the Y direction.
   *
   * Only applicable if "Specify Tiling Scheme" is set to true.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta = (EditCondition = "bSpecifyTilingScheme", ClampMin = 1))
  int32 RootTilesY = 1;

  /**
   * The west boundary of the bounding rectangle used for the quadtree tiling
   * scheme. Specified in longitude degrees in the range [-180, 180].
   *
   * Only applicable if "Specify Tiling Scheme" is set to true.
   */
  UPROPERTY(
      Category = "Cesium",
      EditAnywhere,
      BlueprintReadWrite,
      meta =
          (EditCondition = "bSpecifyTilingScheme",
           ClampMin = -180.0,
           ClampMax = 180.0))
  double RectangleWest = -180;

  /**
   * The south boundary of the bounding rectangle used for the quadtree tiling
   * scheme. Specified in latitude degrees in the range [-90, 90].
   *
   * Only applicable if "Specify Tiling Scheme" is set to true.
   */
  UPROPERTY(
      Category = "Cesium",
      Category = "Cesium",
      EditAnywhere,
      BlueprintReadWrite,
      meta =
          (EditCondition = "bSpecifyTilingScheme",
           ClampMin = -90.0,
           ClampMax = 90.0))
  double RectangleSouth = -90;

  /**
   * The east boundary of the bounding rectangle used for the quadtree tiling
   * scheme. Specified in longitude degrees in the range [-180, 180].
   *
   * Only applicable if "Specify Tiling Scheme" is set to true.
   */
  UPROPERTY(
      Category = "Cesium",
      EditAnywhere,
      BlueprintReadWrite,
      meta =
          (EditCondition = "bSpecifyTilingScheme",
           ClampMin = -180.0,
           ClampMax = 180.0))
  double RectangleEast = 180;

  /**
   * The north boundary of the bounding rectangle used for the quadtree tiling
   * scheme. Specified in latitude degrees in the range [-90, 90].
   *
   * Only applicable if "Specify Tiling Scheme" is set to true.
   */
  UPROPERTY(
      Category = "Cesium",
      EditAnywhere,
      BlueprintReadWrite,
      meta =
          (EditCondition = "bSpecifyTilingScheme",
           ClampMin = -90.0,
           ClampMax = 90.0))
  double RectangleNorth = 90;

  /**
   * Set this to true to directly specify the minimum and maximum zoom levels
   * available from the server. If false, the minimum and maximum zoom levels
   * will be retrieved from the server's tilemapresource.xml file.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  bool bSpecifyZoomLevels = false;

  /**
   * Minimum zoom level.
   *
   * Take care when specifying this that the number of tiles at the minimum
   * level is small, such as four or less. A larger number is likely to result
   * in rendering problems.
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

  /**
   * The pixel width of the image tiles.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta = (ClampMin = 64, ClampMax = 2048))
  int32 TileWidth = 256;

  /**
   * The pixel height of the image tiles.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta = (ClampMin = 64, ClampMax = 2048))
  int32 TileHeight = 256;

  /**
   * HTTP headers to be attached to each request made for this raster overlay.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  TMap<FString, FString> RequestHeaders;

  virtual void Serialize(FArchive& Ar) override;

protected:
  virtual std::unique_ptr<CesiumRasterOverlays::RasterOverlay> CreateOverlay(
      const CesiumRasterOverlays::RasterOverlayOptions& options = {}) override;
};
