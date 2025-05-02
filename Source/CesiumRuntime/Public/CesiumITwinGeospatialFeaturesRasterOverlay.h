// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumRasterOverlay.h"
#include "CesiumVectorDocumentRasterOverlay.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "CesiumITwinGeospatialFeaturesRasterOverlay.generated.h"

UCLASS(ClassGroup = Cesium, meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumITwinGeospatialFeaturesRasterOverlay
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

  /**
   * The ID of the iTwin to load the Geospatial Features Collection from.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FString ITwinID;

  /**
   * The ID of the iTwin Geospatial Features Collection to load features from.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FString CollectionID;

  /**
   * The authentication token to use to access the iTwin API. This can be either
   * an access token or a share token.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FString ITwinToken;

  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      Category = "Cesium",
      meta = (ClampMin = "0", ClampMax = "8"))
  int32 MipLevels = 0;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FCesiumVectorStyle DefaultStyle;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FCesiumVectorDocumentRasterOverlayStyleCallback StyleCallback;

protected:
  virtual std::unique_ptr<CesiumRasterOverlays::RasterOverlay> CreateOverlay(
      const CesiumRasterOverlays::RasterOverlayOptions& options = {}) override;
};
