// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumRasterOverlay.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "CesiumVectorDocument.h"
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
  FCesiumVectorDocument VectorDocument;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FColor Color = FColor(0, 0, 0, 255);

protected:
  virtual std::unique_ptr<CesiumRasterOverlays::RasterOverlay> CreateOverlay(
      const CesiumRasterOverlays::RasterOverlayOptions& options = {}) override;
};
