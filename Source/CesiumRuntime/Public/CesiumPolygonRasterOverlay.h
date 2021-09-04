// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumRasterOverlay.h"
#include "CoreMinimal.h"
#include "CesiumPolygonRasterOverlay.generated.h"

class ACesiumCartographicPolygon;

namespace Cesium3DTilesSelection {
class RasterizedPolygonsTileExcluder;
}

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
  TArray<ACesiumCartographicPolygon*> Polygons;

  /**
   * Whether tiles that fall entirely inside this overlay's polygons should be
   * excluded from loading and rendering. For better performance, this should be
   * enabled when this overlay will be used for clipping. But when this overlay
   * is used for other effects, this option should be disabled to avoid missing
   * tiles.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  bool ExcludeTilesInside = true;

protected:
  virtual std::unique_ptr<Cesium3DTilesSelection::RasterOverlay>
  CreateOverlay() override;
  virtual void OnAdd(
      Cesium3DTilesSelection::Tileset* pTileset,
      Cesium3DTilesSelection::RasterOverlay* pOverlay) override;
  virtual void OnRemove(
      Cesium3DTilesSelection::Tileset* pTileset,
      Cesium3DTilesSelection::RasterOverlay* pOverlay) override;

private:
  std::shared_ptr<Cesium3DTilesSelection::RasterizedPolygonsTileExcluder>
      _pExcluder;
};
