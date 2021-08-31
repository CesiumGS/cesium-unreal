// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumPolygonRasterOverlay.h"
#include "Cesium3DTilesSelection/RasterizedPolygonsOverlay.h"
#include "Cesium3DTilesSelection/RasterizedPolygonsTileExcluder.h"
#include "Cesium3DTilesSelection/Tileset.h"
#include "CesiumBingMapsRasterOverlay.h"
#include "CesiumCartographicPolygon.h"

using namespace CesiumGeospatial;
using namespace Cesium3DTilesSelection;

UCesiumPolygonRasterOverlay::UCesiumPolygonRasterOverlay()
    : UCesiumRasterOverlay() {
  this->MaterialLayerKey = TEXT("Clipping");
}

std::unique_ptr<Cesium3DTilesSelection::RasterOverlay>
UCesiumPolygonRasterOverlay::CreateOverlay() {
  std::vector<CartographicPolygon> polygons;
  polygons.reserve(this->Polygons.Num());

  for (ACesiumCartographicPolygon* pPolygon : this->Polygons) {
    if (!pPolygon) {
      continue;
    }

    CartographicPolygon polygon = pPolygon->CreateCartographicPolygon();
    polygons.emplace_back(std::move(polygon));
  }

  return std::make_unique<Cesium3DTilesSelection::RasterizedPolygonsOverlay>(
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      polygons,
      CesiumGeospatial::Ellipsoid::WGS84,
      CesiumGeospatial::GeographicProjection());
}

void UCesiumPolygonRasterOverlay::OnAdd(
    Tileset* pTileset,
    RasterOverlay* pOverlay) {
  // If this overlay is used for culling, add it as an excluder too for
  // efficiency.
  if (pTileset && this->ExcludeTilesInside) {
    RasterizedPolygonsOverlay* pPolygons =
        static_cast<RasterizedPolygonsOverlay*>(pOverlay);
    assert(this->_pExcluder == nullptr);
    this->_pExcluder =
        std::make_shared<RasterizedPolygonsTileExcluder>(*pPolygons);
    pTileset->getOptions().excluders.push_back(this->_pExcluder);
  }
}

void UCesiumPolygonRasterOverlay::OnRemove(
    Tileset* pTileset,
    RasterOverlay* pOverlay) {
  auto& excluders = pTileset->getOptions().excluders;
  if (this->_pExcluder) {
    auto it = std::find(excluders.begin(), excluders.end(), this->_pExcluder);
    if (it != excluders.end()) {
      excluders.erase(it);
    }

    this->_pExcluder.reset();
  }
}
