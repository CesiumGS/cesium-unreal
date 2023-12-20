// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumWebMapServiceRasterOverlay.h"
#include "Algo/Transform.h"
#include "CesiumRasterOverlays/WebMapServiceRasterOverlay.h"
#include "CesiumRuntime.h"

std::unique_ptr<CesiumRasterOverlays::RasterOverlay>
UCesiumWebMapServiceRasterOverlay::CreateOverlay(
    const CesiumRasterOverlays::RasterOverlayOptions& options) {

  CesiumRasterOverlays::WebMapServiceRasterOverlayOptions wmsOptions;
  if (MaximumLevel > MinimumLevel) {
    wmsOptions.minimumLevel = MinimumLevel;
    wmsOptions.maximumLevel = MaximumLevel;
  }
  wmsOptions.layers = TCHAR_TO_UTF8(*Layers);
  wmsOptions.tileWidth = TileWidth;
  wmsOptions.tileHeight = TileHeight;
  return std::make_unique<CesiumRasterOverlays::WebMapServiceRasterOverlay>(
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      TCHAR_TO_UTF8(*this->BaseUrl),
      std::vector<CesiumAsync::IAssetAccessor::THeader>(),
      wmsOptions,
      options);
}
