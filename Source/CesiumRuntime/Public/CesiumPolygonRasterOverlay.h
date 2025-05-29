// Copyright 2020-2024 CesiumGS, Inc. and Contributors

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
UCLASS(ClassGroup = Cesium, meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumPolygonRasterOverlay
    : public UCesiumRasterOverlay {
  GENERATED_BODY()

public:
  UCesiumPolygonRasterOverlay();

  /**
   * The polygons to rasterize for this overlay.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  TArray<TSoftObjectPtr<ACesiumCartographicPolygon>> Polygons;

  /**
   * Whether to invert the selection specified by the polygons.
   *
   * If this is true, only the areas outside of all the polygons will be
   * rasterized.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  bool InvertSelection = false;

  /**
   * Whether tiles that fall entirely within the rasterized selection should be
   * excluded from loading and rendering. For better performance, this should be
   * enabled when this overlay will be used for clipping. But when this overlay
   * is used for other effects, this option should be disabled to avoid missing
   * tiles.
   *
   * Note that if InvertSelection is true, this will cull tiles that are
   * outside of all the polygons. If it is false, this will cull tiles that are
   * completely inside at least one polygon.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  bool ExcludeSelectedTiles = true;

protected:
  virtual std::unique_ptr<CesiumRasterOverlays::RasterOverlay> CreateOverlay(
      const CesiumRasterOverlays::RasterOverlayOptions& options = {}) override;

  virtual void OnAdd(
      Cesium3DTilesSelection::Tileset* pTileset,
      CesiumRasterOverlays::RasterOverlay* pOverlay) override;
  virtual void OnRemove(
      Cesium3DTilesSelection::Tileset* pTileset,
      CesiumRasterOverlays::RasterOverlay* pOverlay) override;

private:
  std::shared_ptr<Cesium3DTilesSelection::RasterizedPolygonsTileExcluder>
      _pExcluder;
};
