// Fill out your copyright notice in the Description page of Project Settings.


#include "CesiumIonRasterOverlay.h"
#include "Cesium3DTiles/Tileset.h"
#include "Cesium3DTiles/IonRasterOverlay.h"
#include "UnrealConversions.h"

void UCesiumIonRasterOverlay::AddToTileset(Cesium3DTiles::Tileset& tileset) {
    std::unique_ptr<Cesium3DTiles::IonRasterOverlay> pOverlay = std::make_unique<Cesium3DTiles::IonRasterOverlay>(
        this->IonAssetID,
        wstr_to_utf8(this->IonAccessToken)
    );

    for (const FRectangularCutout& cutout : this->Cutouts) {
        pOverlay->getCutouts().push_back(CesiumGeospatial::GlobeRectangle::fromDegrees(cutout.west, cutout.south, cutout.east, cutout.north));
    }

    tileset.getOverlays().push_back(std::move(pOverlay));
}
