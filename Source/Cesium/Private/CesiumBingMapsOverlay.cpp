// Fill out your copyright notice in the Description page of Project Settings.


#include "CesiumBingMapsOverlay.h"
#include "Cesium3DTiles/Tileset.h"
#include "Cesium3DTiles/BingMapsRasterOverlay.h"

void UCesiumBingMapsOverlay::AddToTileset(Cesium3DTiles::Tileset& tileset) {
    tileset.getOverlays().push_back(std::make_unique<Cesium3DTiles::BingMapsRasterOverlay>("https://dev.virtualearth.net", "AmXdbd8UeUJtaRSn7yVwyXgQlBBUqliLbHpgn2c76DfuHwAXfRrgS5qwfHU6Rhm8"));
}
