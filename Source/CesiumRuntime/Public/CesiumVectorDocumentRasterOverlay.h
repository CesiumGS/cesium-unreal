// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumIonServer.h"
#include "CesiumRasterOverlay.h"
#include "CesiumVectorDocument.h"
#include "CesiumVectorStyle.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Delegates/Delegate.h"
#include "CesiumVectorDocumentRasterOverlay.generated.h"

/**
 * The projection used by a CesiumVectorDocumentRasterOverlay.
 */
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

/**
 * Configures where the CesiumVectorDocumentRasterOverlay should load its vector
 * data from.
 */
UENUM(BlueprintType)
enum class ECesiumVectorDocumentRasterOverlaySource : uint8 {
  /**
   * The raster overlay will display the provided VectorDocument.
   */
  FromDocument = 0,
  /**
   * The raster overlay will load the VectorDocument from Cesium ion.
   */
  FromCesiumIon = 1
};

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(
    FCesiumVectorStyle,
    FCesiumVectorDocumentRasterOverlayStyleCallback,
    FCesiumVectorNode,
    InNode);

UCLASS(
    ClassGroup = Cesium,
    BlueprintType,
    Blueprintable,
    meta = (BlueprintSpawnableComponent))
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
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta =
          (EditCondition =
               "Source == ECesiumVectorDocumentRasterOverlaySource::FromCesiumIon"))
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

  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta = (ClampMin = "0", ClampMax = "8"))
  int32 MipLevels = 0;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FCesiumVectorStyle DefaultStyle;

  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium")
  FCesiumVectorDocumentRasterOverlayStyleCallback StyleCallback;

protected:
  virtual std::unique_ptr<CesiumRasterOverlays::RasterOverlay> CreateOverlay(
      const CesiumRasterOverlays::RasterOverlayOptions& options = {}) override;
};
