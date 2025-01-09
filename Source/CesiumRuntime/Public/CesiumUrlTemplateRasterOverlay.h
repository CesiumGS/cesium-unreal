// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumRasterOverlay.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "CesiumUrlTemplateRasterOverlay.generated.h"

/**
 * Specifies the type of projection used for projecting a URL template
 * raster overlay.
 */
UENUM(BlueprintType)
enum class ECesiumUrlTemplateRasterOverlayProjection : uint8 {
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
 * A raster overlay that loads tiles from a templated URL.
 */
UCLASS(ClassGroup = (Cesium), meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumUrlTemplateRasterOverlay
    : public UCesiumRasterOverlay {
  GENERATED_BODY()

public:
  /**
   * @brief The URL containing template parameters that will be substituted when
   * loading tiles.
   *
   * The following template parameters are supported:
   * - `{x}` - The tile X coordinate in the tiling scheme, where 0 is the
   * westernmost tile.
   * - `{y}` - The tile Y coordinate in the tiling scheme, where 0 is the
   * nothernmost tile.
   * - `{z}` - The level of the tile in the tiling scheme, where 0 is the root
   * of the quadtree pyramid.
   * - `{reverseX}` - The tile X coordinate in the tiling scheme, where 0 is the
   * easternmost tile.
   * - `{reverseY}` - The tile Y coordinate in the tiling scheme, where 0 is the
   * southernmost tile.
   * - `{reverseZ}` - The tile Z coordinate in the tiling scheme, where 0 is
   * equivalent to `urlTemplateOptions.maximumLevel`.
   * - `{southDegrees}` - The southern edge of the tile in geodetic degrees.
   * - `{eastDegrees}` - The eastern edge of the tile in geodetic degrees.
   * - `{northDegrees}` - The northern edge of the tile in geodetic degrees.
   * - `{westProjected}` - The western edge of the tile in projected coordinates
   * of the tiling scheme.
   * - `{southProjected}` - The southern edge of the tile in projected
   * coordinates of the tiling scheme.
   * - `{eastProjected}` - The eastern edge of the tile in projected coordinates
   * of the tiling scheme.
   * - `{northProjected}` - The northern edge of the tile in projected
   * coordinates of the tiling scheme.
   * - `{width}` - The width of each tile in pixels.
   * - `{height}` - The height of each tile in pixels.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FString TemplateUrl;

  /**
   * The type of projection used to project the imagery onto the globe.
   * For instance, EPSG:4326 uses geographic projection and EPSG:3857 uses Web
   * Mercator.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  ECesiumUrlTemplateRasterOverlayProjection Projection =
      ECesiumUrlTemplateRasterOverlayProjection::WebMercator;

  /**
   * Set this to true to specify the quadtree tiling scheme according to the
   * specified root tile numbers and projected bounding rectangle. If false, the
   * tiling scheme will be deduced from the projection.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  bool bSpecifyTilingScheme = false;

  /**
   * If specified, this determines the number of tiles at the
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
   * If specified, this determines the number of tiles at the
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

protected:
  virtual std::unique_ptr<CesiumRasterOverlays::RasterOverlay> CreateOverlay(
      const CesiumRasterOverlays::RasterOverlayOptions& options = {}) override;
};
