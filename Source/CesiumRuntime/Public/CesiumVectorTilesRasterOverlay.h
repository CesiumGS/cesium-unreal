// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGeoJsonDocument.h"
#include "CesiumIonServer.h"
#include "CesiumRasterOverlay.h"
#include "CesiumVectorStyle.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Delegates/Delegate.h"
#include "CesiumVectorTilesRasterOverlay.generated.h"


UCLASS(
    ClassGroup = Cesium,
    BlueprintType,
    Blueprintable,
    meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumVectorTilesRasterOverlay
    : public UCesiumRasterOverlay {
  GENERATED_BODY()

public:
  /**
   * A URL to load a GeoJSON document from.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium")
  FString Url;

  /**
   * The number of mip levels to generate for each tile of this raster overlay.
   *
   * Additional mip levels can improve the visual quality of tiles farther from
   * the camera at the cost of additional rasterization time to create each mip
   * level.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta = (ClampMin = "0", ClampMax = "8"))
  int32 MipLevels = 0;

  /**
   * The default style to use for this raster overlay.
   *
   * If no style is set on a GeoJSON object or any of its parents, this style
   * will be used instead.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FCesiumVectorStyle DefaultStyle;

protected:
  virtual std::unique_ptr<CesiumRasterOverlays::RasterOverlay> CreateOverlay(
      const CesiumRasterOverlays::RasterOverlayOptions& options = {}) override;
};
