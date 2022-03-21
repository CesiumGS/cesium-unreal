// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumTileMapServiceRasterOverlay.h"
#include "Cesium3DTilesSelection/TileMapServiceRasterOverlay.h"
#include "CesiumRuntime.h"

std::unique_ptr<Cesium3DTilesSelection::RasterOverlay>
UCesiumTileMapServiceRasterOverlay::CreateOverlay(
    const Cesium3DTilesSelection::RasterOverlayOptions& options) {
  Cesium3DTilesSelection::TileMapServiceRasterOverlayOptions tmsOptions;
  if (MaximumLevel > MinimumLevel && bSpecifyZoomLevels) {
    tmsOptions.minimumLevel = MinimumLevel;
    tmsOptions.maximumLevel = MaximumLevel;
  }
  

  return std::make_unique<Cesium3DTilesSelection::TileMapServiceRasterOverlay>(
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      TCHAR_TO_UTF8(*this->Url),
      TCHAR_TO_UTF8(*this->datasetId),
      TCHAR_TO_UTF8(*this->authorization),
      std::vector<CesiumAsync::IAssetAccessor::THeader>(),
      tmsOptions,
      options);
}
