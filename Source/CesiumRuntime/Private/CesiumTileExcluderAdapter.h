#pragma once
#include "Cesium3DTileset.h"
#include "CesiumTile.h"
#include "CesiumTileExcluder.h"
#include <Cesium3DTilesSelection/ITileExcluder.h>

class CesiumTileExcluderAdapter : public Cesium3DTilesSelection::ITileExcluder {
  virtual bool shouldExclude(
      const Cesium3DTilesSelection::Tile& tile) const noexcept override;
  virtual void startNewFrame() noexcept override;

private:
  UCesiumTileExcluder* Excluder;
  ACesium3DTileset* Tileset;
  UCesiumTile* Tile;
  ACesiumGeoreference* Georeference;
  bool IsExcluderValid;

public:
  CesiumTileExcluderAdapter(
      UCesiumTileExcluder* Excluder,
      ACesium3DTileset* Tileset,
      UCesiumTile* Tile);
};
