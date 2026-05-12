// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumRasterOverlay.h"
#include "CoreMinimal.h"
#include "CesiumGoogleMapTilesRasterOverlay.generated.h"

/**
 * The possible values of the `MapType` property.
 */
UENUM(BlueprintType)
enum class EGoogleMapTilesMapType : uint8 {
  /**
   * Satellite imagery.
   */
  Satellite,

  /**
   * The standard Google Maps painted map tiles.
   */
  Roadmap,

  /**
   * Terrain imagery. When selecting terrain as the map type, you must
   * also add "Roadmap" to the `LayerTypes` property.
   */
  Terrain
};

/**
 * The possible values of the `Scale` property.
 */
UENUM(BlueprintType)
enum class EGoogleMapTilesScale : uint8 {
  /**
   * @brief The default.
   */
  ScaleFactor1x UMETA(DisplayName = "1x"),

  /**
   * @brief Doubles label size and removes minor feature labels.
   */
  ScaleFactor2x UMETA(DisplayName = "2x"),

  /**
   * @brief Quadruples label size and removes minor feature labels.
   */
  ScaleFactor4x UMETA(DisplayName = "4x"),
};

/**
 * The possible values of the `LayerTypes` property.
 */
UENUM(BlueprintType)
enum class EGoogleMapTilesLayerType : uint8 {
  /**
   * Required if you specify terrain as the map type. Can also be
   * optionally overlaid on the satellite map type. Has no effect on roadmap
   * tiles.
   */
  Roadmap,

  /**
   * Shows Street View-enabled streets and locations using blue outlines
   * on the map.
   */
  Streetview,

  /**
   * Displays current traffic conditions.
   */
  Traffic
};

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

  /**
   * The type of base map.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  EGoogleMapTilesMapType MapType = EGoogleMapTilesMapType::Satellite;

  /**
   * An IETF language tag that specifies the language used to display
   * information on the tiles. For example, `en-US` specifies the English
   * language as spoken in the United States.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FString Language = "en-US";

  /**
   * A Common Locale Data Repository region identifier (two uppercase letters)
   * that represents the physical location of the user. For example, `US`.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FString Region = "US";

  /**
   * Scales-up the size of map elements (such as road labels), while
   * retaining the tile size and coverage area of the default tile.
   *
   * Increasing the scale also reduces the number of labels on the map, which
   * reduces clutter.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  EGoogleMapTilesScale Scale = EGoogleMapTilesScale::ScaleFactor1x;

  /**
   * Specifies whether to return high-resolution tiles.
   *
   * If the scale-factor is increased, `highDpi` is used to increase the size of
   * the tile. Normally, increasing the scale factor enlarges the resulting tile
   * into an image of the same size, which lowers quality. With `highDpi`, the
   * resulting size is also increased, preserving quality. DPI stands for Dots
   * per Inch, and High DPI means the tile renders using more dots per inch than
   * normal. If `true`, then the number of pixels in each of the x and y
   * dimensions is multiplied by the scale factor (that is , 2x or 4x). The
   * coverage area of the tile remains unchanged. This parameter works only with
   * `Scale` values of `2x` or `4x`. It has no effect on `1x` scale tiles.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  bool HighDPI = false;

  /**
   * The layer types to be added to the map.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  TArray<EGoogleMapTilesLayerType> LayerTypes;

  /**
   * An array of JSON style objects that specify the appearance and
   * detail level of map features such as roads, parks, and built-up areas.
   *
   * Styling is used to customize the standard Google base map. The `styles`
   * parameter is valid only if the `MapType` is `Roadmap`. For the complete
   * style syntax, see the [Style
   * Reference](https://developers.google.com/maps/documentation/tile/style-reference).
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  TArray<FString> Styles;

  /**
   * Specifies whether `LayerTypes` rendered as a separate overlay, or combined
   * with the base imagery.
   *
   * When `true`, the base map isn't displayed. If you haven't defined any
   * `LayerTypes`, then this value is ignored.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  bool Overlay = false;

protected:
  virtual std::unique_ptr<CesiumRasterOverlays::RasterOverlay> CreateOverlay(
      const CesiumRasterOverlays::RasterOverlayOptions& options = {}) override;
};
