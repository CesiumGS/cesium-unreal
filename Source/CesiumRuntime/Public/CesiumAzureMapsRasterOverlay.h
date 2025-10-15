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
   * All roadmap layers with Azure Maps' main style.
   */
  BaseRoad UMETA(DisplayName = "Base"),
  /**
   * All roadmap layers with Azure Maps' dark grey style.
   */
  BaseDarkGrey UMETA(DisplayName = "Base (Dark Grey)"),
  /**
   * Label data in Azure Maps' main style.
   */
  BaseLabelsRoad UMETA(DisplayName = "Labels"),
  /**
   * Label data in Azure Maps' dark grey style.
   */
  BaseLabelsDarkGrey UMETA(DisplayName = "Labels (Dark Grey)"),
  /**
   * Road, boundary, and label data in Azure Maps' main style.
   */
  BaseHybridRoad UMETA(DisplayName = "Hybrid"),
  /**
   * Road, boundary, and label data in Azure Maps' dark grey style.
   */
  BaseHybridDarkGrey UMETA(DisplayName = "Hybrid (Dark Grey)"),
  /**
   * A combination of satellite or aerial imagery. Only available for accounts
   * under S1 and G2 pricing SKU.
   */
  Imagery,
  /**
   * Shaded relief and terra layers.
   */
  Terra,
  /**
   * Weather radar tiles. Latest weather radar images including areas of rain,
   * snow, ice and mixed conditions.
   */
  WeatherRadar UMETA(DisplayName = "Weather (Radar)"),
  /**
   * Weather infrared tiles. Latest infrared satellite images showing clouds by
   * their temperature.
   */
  WeatherInfrared UMETA(DisplayName = "Weather (Infrared)"),
  /**
   * Absolute traffic tiles in Azure Maps' main style.
   */
  TrafficAbsolute UMETA(DisplayName = "Traffic (Absolute)"),
  /**
   * Relative traffic tiles in Azure Maps' main style. This filters out traffic
   * data from smaller streets that are otherwise included in TrafficAbsolute.
   */
  TrafficRelativeMain UMETA(DisplayName = "Traffic (Relative)"),
  /**
   * Relative traffic tiles in Azure Maps' dark style. This filters out traffic
   * data from smaller streets that are otherwise included in TrafficAbsolute.
   */
  TrafficRelativeDark UMETA(DisplayName = "Traffic (Relative, Dark)"),
  /**
   * Delay traffic tiles in Azure Maps' dark style. This only shows the points
   * of delay along traffic routes that are otherwise included in
   * TrafficAbsolute.
   */
  TrafficDelay UMETA(DisplayName = "Traffic (Delay)"),
  /**
   * Reduced traffic tiles in Azure Maps' dark style. This only shows the
   * traffic routes without the delay points that are otherwise included in
   * TrafficAbsolute.
   */
  TrafficReduced UMETA(DisplayName = "Traffic (Reduced)"),
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
  EAzureMapsTilesetId TilesetId = EAzureMapsTilesetId::BaseRoad;

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
