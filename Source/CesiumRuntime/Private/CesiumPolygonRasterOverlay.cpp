// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumPolygonRasterOverlay.h"
#include "Cesium3DTilesSelection/RasterizedPolygonsTileExcluder.h"
#include "Cesium3DTilesSelection/Tileset.h"
#include "Cesium3DTileset.h"
#include "CesiumBingMapsRasterOverlay.h"
#include "CesiumCartographicPolygon.h"
#include "CesiumRasterOverlays/RasterizedPolygonsOverlay.h"

using namespace Cesium3DTilesSelection;
using namespace CesiumGeospatial;
using namespace CesiumRasterOverlays;

UCesiumPolygonRasterOverlay::UCesiumPolygonRasterOverlay()
    : UCesiumRasterOverlay() {
  this->MaterialLayerKey = TEXT("Clipping");
}

std::unique_ptr<CesiumRasterOverlays::RasterOverlay>
UCesiumPolygonRasterOverlay::CreateOverlay(
    const CesiumRasterOverlays::RasterOverlayOptions& options) {
  ACesium3DTileset* pTileset = this->GetOwner<ACesium3DTileset>();

  FTransform worldToTileset =
      pTileset ? pTileset->GetActorTransform().Inverse() : FTransform::Identity;

  std::vector<CartographicPolygon> polygons;
  polygons.reserve(this->Polygons.Num());

  for (auto& pPolygon : this->Polygons) {
    if (!pPolygon) {
      continue;
    }

    CartographicPolygon polygon =
        pPolygon->CreateCartographicPolygon(worldToTileset);
    polygons.emplace_back(std::move(polygon));
  }

  UCesiumEllipsoid* Ellipsoid = pTileset->ResolveGeoreference()->GetEllipsoid();
  check(IsValid(Ellipsoid));

  return std::make_unique<CesiumRasterOverlays::RasterizedPolygonsOverlay>(
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      polygons,
      this->InvertSelection,
      Ellipsoid->GetNativeEllipsoid(),
      CesiumGeospatial::GeographicProjection(Ellipsoid->GetNativeEllipsoid()),
      options);
}

void UCesiumPolygonRasterOverlay::OnAdd(
    Tileset* pTileset,
    RasterOverlay* pOverlay) {
  // If this overlay is used for culling, add it as an excluder too for
  // efficiency.
  if (pTileset && this->ExcludeSelectedTiles) {
    RasterizedPolygonsOverlay* pPolygons =
        static_cast<RasterizedPolygonsOverlay*>(pOverlay);
    assert(this->_pExcluder == nullptr);
    this->_pExcluder =
        std::make_shared<RasterizedPolygonsTileExcluder>(pPolygons);
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
