// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumDebugColorizeTilesRasterOverlay.h"
#include "Cesium3DTilesSelection/DebugColorizeTilesRasterOverlay.h"
#include "Cesium3DTilesSelection/Tileset.h"

std::unique_ptr<Cesium3DTilesSelection::RasterOverlay>
UCesiumDebugColorizeTilesRasterOverlay::CreateOverlay(
    const Cesium3DTilesSelection::RasterOverlayOptions& options) {
  return std::make_unique<Cesium3DTilesSelection::DebugColorizeTilesRasterOverlay>(
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      options);
}
