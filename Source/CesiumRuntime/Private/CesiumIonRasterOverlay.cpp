// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumIonRasterOverlay.h"
#include "Cesium3DTilesSelection/IonRasterOverlay.h"
#include "Cesium3DTilesSelection/Tileset.h"

std::unique_ptr<Cesium3DTilesSelection::RasterOverlay>
UCesiumIonRasterOverlay::CreateOverlay() {
  return std::make_unique<Cesium3DTilesSelection::IonRasterOverlay>(
      this->IonAssetID,
      TCHAR_TO_UTF8(*this->IonAccessToken));
}
