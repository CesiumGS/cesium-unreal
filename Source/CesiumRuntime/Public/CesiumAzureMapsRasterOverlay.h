// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumRasterOverlay.h"
#include "CoreMinimal.h"
#include "CesiumAzureMapsRasterOverlay.generated.h"

/**
 * Supported values for the `TilesetId` property.
 */
UENUM(BlueprintType)
enum class EAzureMapsTilesetId : uint8 {
  /**
   * @brief All layers with Azure Maps' main style.
   */
  BaseRoad UMETA(DisplayName = "Base"),
  /**
   * @brief All layers with Azure Maps' dark grey style.
   */
  BaseDarkGrey,
  /**
   * @brief Label data in Azure Maps' main style.
   */
  BaseLabelsRoad,
  /**
   * @brief Label data in Azure Maps' dark grey style.
   */
  BaseLabelsDarkGrey,
  /**
   * @brief A combination of satellite or aerial imagery. Only available in S1
   * and G2 pricing SKU.
   */
  Imagery,
  /**
   * @brief Shaded relief and terra layers.
   */
  Terra,
};

/**
 * A raster overlay that directly accesses Azure Maps. If you're using Azure
 * Maps via Cesium ion, use the "Cesium ion Raster Overlay" component instead.
 */
UCLASS(ClassGroup = Cesium, meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumAzureMapsRasterOverlay
    : public UCesiumRasterOverlay {
  GENERATED_BODY()

public:
  /**
   * The Azure Maps subscription key to use.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FString Key;

  /**
   * The version number of Azure Maps API.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FString ApiVersion = "2024-04-01";

  /**
   * The tileset ID to use.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  EAzureMapsTilesetId TilesetId = EAzureMapsTilesetId::Imagery;

  /**
   * The language in which search results should be returned. This should be one
   * of the supported IETF language tags, case insensitive. When data in the
   * specified language is not available for a specific field, default language
   * is used.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FString Language = "en-US";

  /**
   * The View parameter (also called the "user region" parameter) allows
   * you to show the correct maps for a certain country/region for
   * geopolitically disputed regions.
   *
   * Different countries/regions have different views of such regions, and the
   * View parameter allows your application to comply with the view required by
   * the country/region your application will be serving. By default, the View
   * parameter is set to "Unified" even if you haven't defined it in the
   * request. It is your responsibility to determine the location of your users,
   * and then set the View parameter correctly for that location. Alternatively,
   * you have the option to set 'View=Auto', which will return the map data
   * based on the IP address of the request. The View parameter in Azure Maps
   * must be used in compliance with applicable laws, including those regarding
   * mapping, of the country/region where maps, images and other data and third
   * party content that you are authorized to access via Azure Maps is made
   * available. Example: view=IN.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FString View = "US";

protected:
  virtual std::unique_ptr<CesiumRasterOverlays::RasterOverlay> CreateOverlay(
      const CesiumRasterOverlays::RasterOverlayOptions& options = {}) override;
};
