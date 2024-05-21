// Copyright github.com/spr1ngd

#pragma once
#include "CesiumRasterOverlay.h"
#include "CesiumTileMapRasterOverlay.generated.h"

// author : spr1ngd
// desc   : add XYZ format tile map load

UCLASS(ClassGroup = (Cesium), meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumTileMapRasterOverlay : public UCesiumRasterOverlay
{
    GENERATED_BODY()
public:
    /**
     * The base URL of the XYZ-Format Tile Map.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
    FString Url;

    /**
     * The file extension of tile map.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
    FString Format = ".png";

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
    bool bSpecifyZoomLevels = false;

    /**
     * Minimum zoom level.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium", meta = (EditCondition = "bSpecifyZoomLevels", ClampMin = 0))
    int32 MinimumLevel = 0;

    /**
     * Maximum zoom level.
     */
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Cesium",meta = (EditCondition = "bSpecifyZoomLevels", ClampMin = 0))
    int32 MaximumLevel = 10;

    /**
     * Flip tile map y.
     */
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category = "Cesium")
    bool bFlipY = false;

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
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Cesium", meta = (EditCondition = "bSpecifyTilingScheme", ClampMin = 1))
    int32 RootTilesX = 1;

    /**
     * Tile number corresponding to TileRow, also known as TileMatrixHeight.
     */
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Cesium", meta = (EditCondition = "bSpecifyTilingScheme", ClampMin = 1))
    int32 RootTilesY = 1;

    /**
     * The longitude of the west boundary on globe in degrees, in the range
     * [-180, 180]
     */
    UPROPERTY( Category = "Cesium", EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bSpecifyTilingScheme", ClampMin = -180.0, ClampMax = 180.0))
    double West = -180;

    /**
     * The latitude of the south boundary on globe in degrees, in the range
     * [-90, 90]
     */
    UPROPERTY( Category = "Cesium", EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bSpecifyTilingScheme", ClampMin = -90.0, ClampMax = 90.0))
    double South = -90;

    /**
     * The longitude of the west boundary in degrees, in the range [-180, 180]
     */
    UPROPERTY( Category = "Cesium", EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bSpecifyTilingScheme", ClampMin = -180.0, ClampMax = 180.0))
    double East = 180;

    /**
     * The latitude of the south boundary in degrees, in the range [-90, 90]
     */
    UPROPERTY( Category = "Cesium", EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bSpecifyTilingScheme", ClampMin = -90.0, ClampMax = 90.0))
    double North = 90;

protected:

    virtual std::unique_ptr<CesiumRasterOverlays::RasterOverlay> CreateOverlay(const CesiumRasterOverlays::RasterOverlayOptions& options = {}) override;
};
