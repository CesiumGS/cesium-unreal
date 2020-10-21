// Fill out your copyright notice in the Description page of Project Settings.


#include "CesiumIonRasterOverlay.h"
#include "Cesium3DTiles/Tileset.h"
#include "Cesium3DTiles/IonRasterOverlay.h"
#include "UnrealConversions.h"

void UCesiumIonRasterOverlay::AddToTileset(Cesium3DTiles::Tileset& tileset) {
    tileset.getOverlays().push_back(std::make_unique<Cesium3DTiles::IonRasterOverlay>(
        this->IonAssetID,
        wstr_to_utf8(this->IonAccessToken)
    ));   
}
