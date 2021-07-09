// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumIonRasterOverlay.h"
#include "Cesium3DTiles/IonRasterOverlay.h"
#include "Cesium3DTiles/Tileset.h"

std::unique_ptr<Cesium3DTiles::RasterOverlay>
UCesiumIonRasterOverlay::CreateOverlay() {
  return std::make_unique<Cesium3DTiles::IonRasterOverlay>(
      TCHAR_TO_UTF8(*this->Name),
      this->IonAssetID,
      TCHAR_TO_UTF8(*this->IonAccessToken));
}
