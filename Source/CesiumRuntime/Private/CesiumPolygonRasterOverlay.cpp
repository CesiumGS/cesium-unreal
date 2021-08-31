// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumPolygonRasterOverlay.h"
#include "Cesium3DTilesSelection/RasterizedPolygonsOverlay.h"
#include "CesiumBingMapsRasterOverlay.h"
#include "CesiumCartographicSelection.h"

using namespace CesiumGeospatial;

UCesiumPolygonRasterOverlay::UCesiumPolygonRasterOverlay()
    : UCesiumRasterOverlay() {
  this->MaterialLayerKey = TEXT("Clipping");
}

std::unique_ptr<Cesium3DTilesSelection::RasterOverlay>
UCesiumPolygonRasterOverlay::CreateOverlay() {
  std::vector<CartographicPolygon> polygons;
  polygons.reserve(this->Polygons.Num());

  for (ACesiumCartographicSelection* pPolygon : this->Polygons) {
    if (!pPolygon) {
      continue;
    }

    pPolygon->UpdateSelection();
    CartographicPolygon polygon = pPolygon->CreateCesiumCartographicSelection();
    polygons.emplace_back(std::move(polygon));
  }

  return std::make_unique<Cesium3DTilesSelection::RasterizedPolygonsOverlay>(
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      polygons,
      CesiumGeospatial::Ellipsoid::WGS84,
      CesiumGeospatial::GeographicProjection());
}
