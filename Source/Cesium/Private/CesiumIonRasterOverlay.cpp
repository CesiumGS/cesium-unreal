// Fill out your copyright notice in the Description page of Project Settings.


#include "CesiumIonRasterOverlay.h"
#include "Cesium3DTiles/Tileset.h"
#include "Cesium3DTiles/IonRasterOverlay.h"
#include "UnrealConversions.h"

std::unique_ptr<Cesium3DTiles::RasterOverlay> UCesiumIonRasterOverlay::CreateOverlay() {
    return std::make_unique<Cesium3DTiles::IonRasterOverlay>(
        this->IonAssetID,
        wstr_to_utf8(this->IonAccessToken)
    );
}
