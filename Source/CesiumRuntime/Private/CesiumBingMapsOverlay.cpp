// Copyright 2020-2021 CesiumGS, Inc. and Contributors


#include "CesiumBingMapsOverlay.h"
#include "Cesium3DTiles/Tileset.h"
#include "Cesium3DTiles/BingMapsRasterOverlay.h"
#include "UnrealConversions.h"

std::unique_ptr<Cesium3DTiles::RasterOverlay> UCesiumBingMapsOverlay::CreateOverlay() {
    std::string mapStyle;

    switch (this->MapStyle) {
    case EBingMapsStyle::Aerial:
        mapStyle = Cesium3DTiles::BingMapsStyle::AERIAL;
        break;
    case EBingMapsStyle::AerialWithLabelsOnDemand:
        mapStyle = Cesium3DTiles::BingMapsStyle::AERIAL_WITH_LABELS_ON_DEMAND;
        break;
    case EBingMapsStyle::RoadOnDemand:
        mapStyle = Cesium3DTiles::BingMapsStyle::ROAD_ON_DEMAND;
        break;
    case EBingMapsStyle::CanvasDark:
        mapStyle = Cesium3DTiles::BingMapsStyle::CANVAS_DARK;
        break;
    case EBingMapsStyle::CanvasLight:
        mapStyle = Cesium3DTiles::BingMapsStyle::CANVAS_LIGHT;
        break;
    case EBingMapsStyle::CanvasGray:
        mapStyle = Cesium3DTiles::BingMapsStyle::CANVAS_GRAY;
        break;
    case EBingMapsStyle::OrdnanceSurvey:
        mapStyle = Cesium3DTiles::BingMapsStyle::ORDNANCE_SURVEY;
        break;
    case EBingMapsStyle::CollinsBart:
        mapStyle = Cesium3DTiles::BingMapsStyle::COLLINS_BART;
        break;
    }

    return std::make_unique<Cesium3DTiles::BingMapsRasterOverlay>(
        "https://dev.virtualearth.net",
        wstr_to_utf8(this->BingMapsKey),
        mapStyle,
        "",
        CesiumGeospatial::Ellipsoid::WGS84
    );
}
