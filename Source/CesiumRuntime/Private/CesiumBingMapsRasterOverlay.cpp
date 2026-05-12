// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumBingMapsRasterOverlay.h"
#include "Cesium3DTilesSelection/Tileset.h"
#include "CesiumRasterOverlays/BingMapsRasterOverlay.h"

std::unique_ptr<CesiumRasterOverlays::RasterOverlay>
UCesiumBingMapsRasterOverlay::CreateOverlay(
    const CesiumRasterOverlays::RasterOverlayOptions& options) {
  std::string mapStyle;

  switch (this->MapStyle) {
  case EBingMapsStyle::Aerial:
    mapStyle = CesiumRasterOverlays::BingMapsStyle::AERIAL;
    break;
  case EBingMapsStyle::AerialWithLabelsOnDemand:
    mapStyle =
        CesiumRasterOverlays::BingMapsStyle::AERIAL_WITH_LABELS_ON_DEMAND;
    break;
  case EBingMapsStyle::RoadOnDemand:
    mapStyle = CesiumRasterOverlays::BingMapsStyle::ROAD_ON_DEMAND;
    break;
  case EBingMapsStyle::CanvasDark:
    mapStyle = CesiumRasterOverlays::BingMapsStyle::CANVAS_DARK;
    break;
  case EBingMapsStyle::CanvasLight:
    mapStyle = CesiumRasterOverlays::BingMapsStyle::CANVAS_LIGHT;
    break;
  case EBingMapsStyle::CanvasGray:
    mapStyle = CesiumRasterOverlays::BingMapsStyle::CANVAS_GRAY;
    break;
  case EBingMapsStyle::OrdnanceSurvey:
    mapStyle = CesiumRasterOverlays::BingMapsStyle::ORDNANCE_SURVEY;
    break;
  case EBingMapsStyle::CollinsBart:
    mapStyle = CesiumRasterOverlays::BingMapsStyle::COLLINS_BART;
    break;
  }

  return std::make_unique<CesiumRasterOverlays::BingMapsRasterOverlay>(
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      "https://dev.virtualearth.net",
      TCHAR_TO_UTF8(*this->BingMapsKey),
      mapStyle,
      "",
      options);
}
