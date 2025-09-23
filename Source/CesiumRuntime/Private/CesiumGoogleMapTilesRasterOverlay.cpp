// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#include "CesiumGoogleMapTilesRasterOverlay.h"
#include "Cesium3DTilesSelection/Tileset.h"
#include "CesiumRasterOverlays/GoogleMapTilesRasterOverlay.h"

std::unique_ptr<CesiumRasterOverlays::RasterOverlay>
UCesiumGoogleMapTilesRasterOverlay::CreateOverlay(
    const CesiumRasterOverlays::RasterOverlayOptions& options) {
  return std::make_unique<CesiumRasterOverlays::GoogleMapTilesRasterOverlay>(
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      TCHAR_TO_UTF8(*this->Key),
      "satellite",
      "en-US",
      "US",
      CesiumRasterOverlays::GoogleMapTilesOptions{},
      options);
}
