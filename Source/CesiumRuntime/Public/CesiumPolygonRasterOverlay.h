// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumRasterOverlay.h"
#include "CoreMinimal.h"
#include "CesiumPolygonRasterOverlay.generated.h"

class ACesiumCartographicSelection;

/**
 * A raster overlay that rasterizes polygons and drapes them over the tileset.
 * This is useful for clipping out parts of a tileset, for adding a water effect
 * in an area, and for many other purposes.
 */
UCLASS(ClassGroup = (Cesium), meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumPolygonRasterOverlay
    : public UCesiumRasterOverlay {
  GENERATED_BODY()

public:
  UCesiumPolygonRasterOverlay();

  /**
   * The polygons to rasterize for this overlay.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  TArray<ACesiumCartographicSelection*> Polygons;

protected:
  virtual std::unique_ptr<Cesium3DTilesSelection::RasterOverlay>
  CreateOverlay() override;
};
