#include "CesiumTileExcluderAdapter.h"
#include "VecMath.h"

bool CesiumTileExcluderAdapter::shouldExclude(
    const Cesium3DTilesSelection::Tile& tile) const noexcept {
  if (!this->IsExcluderValid) {
    return false;
  }
  Tile->_pTile = &tile;
  return Excluder->ShouldExclude(Tile);
}

void CesiumTileExcluderAdapter::startNewFrame() noexcept {
  if (!IsValid(Tile) || !IsValid(Georeference) || !IsValid(Excluder) ||
      !IsValid(Tileset)) {
    IsExcluderValid = false;
    return;
  }

  IsExcluderValid = true;
  Tile->_transform =
      VecMath::createMatrix4D(Tileset->GetTransform().ToMatrixWithScale()) *
      Georeference->GetGeoTransforms()
          .GetEllipsoidCenteredToAbsoluteUnrealWorldTransform();
}

CesiumTileExcluderAdapter::CesiumTileExcluderAdapter(
    UCesiumTileExcluder* pExcluder,
    ACesium3DTileset* pTileset,
    UCesiumTile* pTile)
    : Excluder(pExcluder),
      Tileset(pTileset),
      Tile(pTile),
      Georeference(pTileset->ResolveGeoreference()),
      IsExcluderValid(true){};
