// Fill out your copyright notice in the Description page of Project Settings.


#include "CesiumBingMapsOverlay.h"
#include "Cesium3DTiles/Tileset.h"
#include "Cesium3DTiles/BingMapsRasterOverlay.h"
#include "UnrealConversions.h"

void UCesiumBingMapsOverlay::AddToTileset(Cesium3DTiles::Tileset& tileset) {
    if (this->IonAssetID > 0) {
        tileset.getOverlays().push_back(std::make_unique<Cesium3DTiles::BingMapsRasterOverlay>(
            this->IonAssetID,
            wstr_to_utf8(this->IonAccessToken)
        ));
    } else {
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

        tileset.getOverlays().push_back(std::make_unique<Cesium3DTiles::BingMapsRasterOverlay>(
            "https://dev.virtualearth.net",
            wstr_to_utf8(this->BingMapsKey),
            mapStyle,
            "",
            CesiumGeospatial::Ellipsoid::WGS84
        ));
    }
    
}
