// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumIonRasterOverlay.h"
#include "Cesium3DTiles/IonRasterOverlay.h"
#include "Cesium3DTiles/Tileset.h"
#include "UnrealConversions.h"

std::unique_ptr<Cesium3DTiles::RasterOverlay>
UCesiumIonRasterOverlay::CreateOverlay() {
  return std::make_unique<Cesium3DTiles::IonRasterOverlay>(
      this->IonAssetID,
      wstr_to_utf8(this->IonAccessToken));
}
