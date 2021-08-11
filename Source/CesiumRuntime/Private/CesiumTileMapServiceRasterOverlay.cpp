// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumTileMapServiceRasterOverlay.h"
#include "Cesium3DTilesSelection/TileMapServiceRasterOverlay.h"
#include "CesiumRuntime.h"

std::unique_ptr<Cesium3DTilesSelection::RasterOverlay>
UCesiumTileMapServiceRasterOverlay::CreateOverlay() {
  Cesium3DTilesSelection::TileMapServiceRasterOverlayOptions Options;
  if (MaximumLevel > MinimumLevel && bSpecifyZoomLevels) {
    Options.minimumLevel = MinimumLevel;
    Options.maximumLevel = MaximumLevel;
  }
  return std::make_unique<Cesium3DTilesSelection::TileMapServiceRasterOverlay>(
      TCHAR_TO_UTF8(*this->Name),
      TCHAR_TO_UTF8(*this->Url),
      std::vector<CesiumAsync::IAssetAccessor::THeader>(),
      Options);
}
