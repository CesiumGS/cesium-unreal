// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumTileMapServiceRasterOverlay.h"
#include "Cesium3DTiles/TileMapServiceRasterOverlay.h"
#include "CesiumRuntime.h"

std::unique_ptr<Cesium3DTiles::RasterOverlay>
UCesiumTileMapServiceRasterOverlay::CreateOverlay() {
  Cesium3DTiles::TileMapServiceRasterOverlayOptions Options;
  if (MaximumLevel > MinimumLevel && bSpecifyZoomLevels) {
    Options.minimumLevel = MinimumLevel;
    Options.maximumLevel = MaximumLevel;
  }
  return std::make_unique<Cesium3DTiles::TileMapServiceRasterOverlay>(
      TCHAR_TO_UTF8(*this->Name),
      TCHAR_TO_UTF8(*this->Url),
      std::vector<CesiumAsync::IAssetAccessor::THeader>(),
      Options);
}
