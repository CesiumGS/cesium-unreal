// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumBingMapsRasterOverlay.h"
#include "Cesium3DTilesSelection/BingMapsRasterOverlay.h"
#include "Cesium3DTilesSelection/Tileset.h"

std::unique_ptr<Cesium3DTilesSelection::RasterOverlay>
UCesiumBingMapsRasterOverlay::CreateOverlay() {
  std::string mapStyle;

  switch (this->MapStyle) {
  case EBingMapsStyle::Aerial:
    mapStyle = Cesium3DTilesSelection::BingMapsStyle::AERIAL;
    break;
  case EBingMapsStyle::AerialWithLabelsOnDemand:
    mapStyle =
        Cesium3DTilesSelection::BingMapsStyle::AERIAL_WITH_LABELS_ON_DEMAND;
    break;
  case EBingMapsStyle::RoadOnDemand:
    mapStyle = Cesium3DTilesSelection::BingMapsStyle::ROAD_ON_DEMAND;
    break;
  case EBingMapsStyle::CanvasDark:
    mapStyle = Cesium3DTilesSelection::BingMapsStyle::CANVAS_DARK;
    break;
  case EBingMapsStyle::CanvasLight:
    mapStyle = Cesium3DTilesSelection::BingMapsStyle::CANVAS_LIGHT;
    break;
  case EBingMapsStyle::CanvasGray:
    mapStyle = Cesium3DTilesSelection::BingMapsStyle::CANVAS_GRAY;
    break;
  case EBingMapsStyle::OrdnanceSurvey:
    mapStyle = Cesium3DTilesSelection::BingMapsStyle::ORDNANCE_SURVEY;
    break;
  case EBingMapsStyle::CollinsBart:
    mapStyle = Cesium3DTilesSelection::BingMapsStyle::COLLINS_BART;
    break;
  }

  return std::make_unique<Cesium3DTilesSelection::BingMapsRasterOverlay>(
      "https://dev.virtualearth.net",
      TCHAR_TO_UTF8(*this->BingMapsKey),
      mapStyle,
      "",
      CesiumGeospatial::Ellipsoid::WGS84);
}
