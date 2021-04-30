// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumTMSRasterOverlay.h"

#include "CesiumRuntime.h"
#include <Cesium3DTiles/TileMapServiceRasterOverlay.h>

std::unique_ptr<Cesium3DTiles::RasterOverlay>
UCesiumTMSRasterOverlay::CreateOverlay() {
  Cesium3DTiles::TileMapServiceRasterOverlayOptions Options;
  if (MaximumLevel > MinimumLevel && bClampWithDefinedZoomLevels) {
    Options.minimumLevel = MinimumLevel;
    Options.maximumLevel = MaximumLevel;
  }
  return std::make_unique<Cesium3DTiles::TileMapServiceRasterOverlay>(
      TCHAR_TO_UTF8(*this->SourceURL),
      std::vector<CesiumAsync::IAssetAccessor::THeader>(),
      Options);
}
