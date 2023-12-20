// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumDebugColorizeTilesRasterOverlay.h"
#include "Cesium3DTilesSelection/Tileset.h"
#include "CesiumRasterOverlays/DebugColorizeTilesRasterOverlay.h"

std::unique_ptr<CesiumRasterOverlays::RasterOverlay>
UCesiumDebugColorizeTilesRasterOverlay::CreateOverlay(
    const CesiumRasterOverlays::RasterOverlayOptions& options) {
  return std::make_unique<
      CesiumRasterOverlays::DebugColorizeTilesRasterOverlay>(
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      options);
}
