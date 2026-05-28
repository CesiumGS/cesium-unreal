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

/**
 * Configures where the CesiumVectorTilesRasterOverlay should load its vector
 * data from.
 */
UENUM(BlueprintType)
enum class ECesiumVectorTilesRasterOverlaySource : uint8 {
  /**
   * The raster overlay will load a vector tileset from Cesium ion.
   */
  FromCesiumIon = 0,
  /**
   * The raster overlay will load a vector tileset from a URL.
   */
  FromUrl = 1
};

UCLASS(
    ClassGroup = Cesium,
    BlueprintType,
    Blueprintable,
    meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumVectorTilesRasterOverlay
    : public UCesiumRasterOverlay {
  GENERATED_BODY()

public:
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  ECesiumVectorTilesRasterOverlaySource Source =
      ECesiumVectorTilesRasterOverlaySource::FromCesiumIon;
  /**
   * The ID of the Cesium ion asset to use.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta =
          (EditCondition =
               "Source == ECesiumVectorTilesRasterOverlaySource::FromCesiumIon"))
  int64 IonAssetID;

  /**
   * The access token to use to access the Cesium ion resource.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta =
          (EditCondition =
               "Source == ECesiumVectorTilesRasterOverlaySource::FromCesiumIon"))
  FString IonAccessToken;

  /**
   * The Cesium ion Server from which this raster overlay is loaded.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      AdvancedDisplay,
      meta =
          (EditCondition =
               "Source == ECesiumVectorTilesRasterOverlaySource::FromCesiumIon"))
  UCesiumIonServer* CesiumIonServer;

  /**
   * A URL to load a vector tiles tileset from.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta =
          (EditCondition =
               "Source == ECesiumVectorTilesRasterOverlaySource::FromUrl"))
  FString Url;

  /**
   * Headers to use while making a request to `Url` to load a vector tiles
   * tileset.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta =
          (EditCondition =
               "Source == ECesiumVectorTilesRasterOverlaySource::FromUrl"))
  TMap<FString, FString> RequestHeaders;

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
   * If no style information is present in the vector tiles tileset, this style
   * will be used instead.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FCesiumVectorStyle DefaultStyle;

protected:
  virtual std::unique_ptr<CesiumRasterOverlays::RasterOverlay> CreateOverlay(
      const CesiumRasterOverlays::RasterOverlayOptions& options = {}) override;
};
