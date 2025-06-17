// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGeoJsonDocument.h"
#include "CesiumIonServer.h"
#include "CesiumRasterOverlay.h"
#include "CesiumVectorStyle.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Delegates/Delegate.h"
#include "CesiumGeoJsonDocumentRasterOverlay.generated.h"

/**
 * The projection used by a CesiumVectorDocumentRasterOverlay.
 */
UENUM(BlueprintType)
enum class ECesiumGeoJsonDocumentRasterOverlayProjection : uint8 {
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
 * Configures where the CesiumVectorDocumentRasterOverlay should load its vector
 * data from.
 */
UENUM(BlueprintType)
enum class ECesiumGeoJsonDocumentRasterOverlaySource : uint8 {
  /**
   * The raster overlay will display the provided GeoJsonDocument.
   */
  FromDocument = 0,
  /**
   * The raster overlay will load a GeoJsonDocument from Cesium ion.
   */
  FromCesiumIon = 1,
  /**
   * The raster overlay will load a GeoJsonDocument from a URL.
   */
  FromUrl = 2
};

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCesiumGeoJsonDocumentRasterOverlayOnDocumentLoadedCallback,
    FCesiumGeoJsonDocument,
    InDocument);

UCLASS(
    ClassGroup = Cesium,
    BlueprintType,
    Blueprintable,
    meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumGeoJsonDocumentRasterOverlay
    : public UCesiumRasterOverlay {
  GENERATED_BODY()

public:
  /**
   * The type of projection used to project the imagery onto the globe.
   * For instance, EPSG:4326 uses geographic projection and EPSG:3857 uses Web
   * Mercator.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  ECesiumGeoJsonDocumentRasterOverlayProjection Projection =
      ECesiumGeoJsonDocumentRasterOverlayProjection::WebMercator;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  ECesiumGeoJsonDocumentRasterOverlaySource Source =
      ECesiumGeoJsonDocumentRasterOverlaySource::FromCesiumIon;

  /**
   * The ID of the Cesium ion asset to use.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta =
          (EditCondition =
               "Source == ECesiumGeoJsonDocumentRasterOverlaySource::FromCesiumIon"))
  int64 IonAssetID;

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
               "Source == ECesiumGeoJsonDocumentRasterOverlaySource::FromCesiumIon"))
  UCesiumIonServer* CesiumIonServer;

  /**
   * A FCesiumGeoJsonDocument to display.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta =
          (EditCondition =
               "Source == ECesiumGeoJsonDocumentRasterOverlaySource::FromDocument"))
  FCesiumGeoJsonDocument GeoJsonDocument;

  /**
   * A URL to load a GeoJSON document from.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta =
          (EditCondition =
               "Source == ECesiumGeoJsonDocumentRasterOverlaySource::FromUrl"))
  FString Url;

  /**
   * Headers to use while making a request to `Url` to load a GeoJSON document.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta =
          (EditCondition =
               "Source == ECesiumGeoJsonDocumentRasterOverlaySource::FromUrl"))
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
   * If no style is set on a GeoJSON object or any of its parents, this style
   * will be used instead.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FCesiumVectorStyle DefaultStyle;

  /**
   * A callback that will be called when the document has been loaded.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FCesiumGeoJsonDocumentRasterOverlayOnDocumentLoadedCallback OnDocumentLoaded;

protected:
  virtual std::unique_ptr<CesiumRasterOverlays::RasterOverlay> CreateOverlay(
      const CesiumRasterOverlays::RasterOverlayOptions& options = {}) override;
};
