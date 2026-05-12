// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once
#include "CesiumTile.h"
#include "CesiumTileExcluder.h"
#include <Cesium3DTilesSelection/ITileExcluder.h>

class ACesiumGeoreference;

namespace Cesium3DTilesSelection {
class Tile;
}

class CesiumTileExcluderAdapter : public Cesium3DTilesSelection::ITileExcluder {
  virtual bool shouldExclude(
      const Cesium3DTilesSelection::Tile& tile) const noexcept override;
  virtual void startNewFrame() noexcept override;

private:
  TWeakObjectPtr<UCesiumTileExcluder> Excluder;
  UCesiumTile* Tile;
  ACesiumGeoreference* Georeference;
  bool IsExcluderValid;

public:
  CesiumTileExcluderAdapter(
      TWeakObjectPtr<UCesiumTileExcluder> pExcluder,
      ACesiumGeoreference* pGeoreference,
      UCesiumTile* pTile);
};
