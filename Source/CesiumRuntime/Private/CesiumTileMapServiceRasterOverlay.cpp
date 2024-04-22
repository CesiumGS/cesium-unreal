// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumTileMapServiceRasterOverlay.h"
#include "CesiumRasterOverlays/TileMapServiceRasterOverlay.h"
#include "CesiumRuntime.h"

std::unique_ptr<CesiumRasterOverlays::RasterOverlay>
UCesiumTileMapServiceRasterOverlay::CreateOverlay(
    const CesiumRasterOverlays::RasterOverlayOptions& options) {
  CesiumRasterOverlays::TileMapServiceRasterOverlayOptions tmsOptions;
  if (MaximumLevel > MinimumLevel && bSpecifyZoomLevels) {
    tmsOptions.minimumLevel = MinimumLevel;
    tmsOptions.maximumLevel = MaximumLevel;
  }
  return std::make_unique<CesiumRasterOverlays::TileMapServiceRasterOverlay>(
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      TCHAR_TO_UTF8(*this->Url),
      std::vector<CesiumAsync::IAssetAccessor::THeader>(),
      tmsOptions,
      options);
}
