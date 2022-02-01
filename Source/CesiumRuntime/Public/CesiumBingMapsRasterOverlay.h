// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumRasterOverlay.h"
#include "CoreMinimal.h"
#include "CesiumBingMapsRasterOverlay.generated.h"

UENUM(BlueprintType)
enum class EBingMapsStyle : uint8 {
  Aerial UMETA(DisplayName = "Aerial"),
  AerialWithLabelsOnDemand UMETA(DisplayName = "Aerial with Labels"),
  RoadOnDemand UMETA(DisplayName = "Road"),
  CanvasDark UMETA(DisplayName = "Canvas Dark"),
  CanvasLight UMETA(DisplayName = "Canvas Light"),
  CanvasGray UMETA(DisplayName = "Canvas Gray"),
  OrdnanceSurvey UMETA(DisplayName = "Ordnance Survey"),
  CollinsBart UMETA(DisplayName = "Collins Bart")
};

/**
 * A raster overlay that directly accesses Bing Maps. If you're using Bing Maps
 * via Cesium ion, use the "Cesium ion Raster Overlay" component instead.
 */
UCLASS(ClassGroup = (Cesium), meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumBingMapsRasterOverlay
    : public UCesiumRasterOverlay {
  GENERATED_BODY()

public:
  /**
   * The Bing Maps API key to use.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FString BingMapsKey;

  /**
   * The map style to use.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  EBingMapsStyle MapStyle = EBingMapsStyle::Aerial;

protected:
  virtual std::unique_ptr<Cesium3DTilesSelection::RasterOverlay> CreateOverlay(
      const Cesium3DTilesSelection::RasterOverlayOptions& options = {})
      override;
};
