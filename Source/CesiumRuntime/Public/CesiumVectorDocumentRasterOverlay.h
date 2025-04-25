// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumIonServer.h"
#include "CesiumRasterOverlay.h"
#include "CesiumVectorDocument.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "CesiumVectorDocumentRasterOverlay.generated.h"

UENUM(BlueprintType)
enum class ECesiumVectorDocumentRasterOverlayProjection : uint8 {
  /**
   * The raster overlay is projected using Web Mercator.
   */
  WebMercator,

  /**
   * The raster overlay is projected using a geographic projection.
   */
  Geographic
};

UENUM(BlueprintType)
enum class ECesiumVectorDocumentRasterOverlaySource : uint8 {
  FromDocument = 0,
  FromCesiumIon = 1
};

UENUM(BlueprintType)
enum class ECesiumVectorDocumentRasterOverlayLineWidthMode : uint8 {
  Pixels = 0,
  Meters = 1
};

UCLASS(ClassGroup = Cesium, meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumVectorDocumentRasterOverlay
    : public UCesiumRasterOverlay {
  GENERATED_BODY()

public:
  /**
   * The type of projection used to project the imagery onto the globe.
   * For instance, EPSG:4326 uses geographic projection and EPSG:3857 uses Web
   * Mercator.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  ECesiumVectorDocumentRasterOverlayProjection Projection =
      ECesiumVectorDocumentRasterOverlayProjection::WebMercator;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  ECesiumVectorDocumentRasterOverlaySource Source =
      ECesiumVectorDocumentRasterOverlaySource::FromCesiumIon;

  /**
   * The ID of the Cesium ion asset to use.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium", meta=(EditCondition="Source == ECesiumVectorDocumentRasterOverlaySource::FromCesiumIon"))
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
               "Source == ECesiumVectorDocumentRasterOverlaySource::FromCesiumIon"))
  UCesiumIonServer* CesiumIonServer;

  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta =
          (EditCondition =
               "Source == ECesiumVectorDocumentRasterOverlaySource::FromDocument"))
  FCesiumVectorDocument VectorDocument;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FColor Color = FColor(0, 0, 0, 255);

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  double LineWidth = 1.0;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  ECesiumVectorDocumentRasterOverlayLineWidthMode LineWidthMode =
      ECesiumVectorDocumentRasterOverlayLineWidthMode::Pixels;

protected:
  virtual std::unique_ptr<CesiumRasterOverlays::RasterOverlay> CreateOverlay(
      const CesiumRasterOverlays::RasterOverlayOptions& options = {}) override;
};
