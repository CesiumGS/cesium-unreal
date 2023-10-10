// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumWebMapTileServiceRasterOverlay.h"
#include "Cesium3DTilesSelection/WebMapTileServiceRasterOverlay.h"
#include "CesiumRuntime.h"

std::unique_ptr<Cesium3DTilesSelection::RasterOverlay>
UCesiumWebMapTileServiceRasterOverlay::CreateOverlay(
    const Cesium3DTilesSelection::RasterOverlayOptions& options) {
  Cesium3DTilesSelection::WebMapTileServiceRasterOverlayOptions wmtsOptions;
  if (!TileMatrixSetID.IsEmpty()) {
    wmtsOptions.tileMatrixSetID = TCHAR_TO_UTF8(*this->TileMatrixSetID);
  }
  if (!Style.IsEmpty()) {
    wmtsOptions.style = TCHAR_TO_UTF8(*this->Style);
  }
  if (!Layer.IsEmpty()) {
    wmtsOptions.layer = TCHAR_TO_UTF8(*this->Layer);
  }
  if (MaximumLevel > MinimumLevel && bSpecifyZoomLevels) {
    wmtsOptions.minimumLevel = MinimumLevel;
    wmtsOptions.maximumLevel = MaximumLevel;
  }
  if (!TileMatrixSetLabels.IsEmpty()) {
    std::vector<std::string> labels;
    for (const auto& label : this->TileMatrixSetLabels) {
      labels.emplace_back(TCHAR_TO_UTF8(*label));
    }
    wmtsOptions.tileMatrixLabels = labels;
  }
  return std::make_unique<
      Cesium3DTilesSelection::WebMapTileServiceRasterOverlay>(
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      TCHAR_TO_UTF8(*this->Url),
      std::vector<CesiumAsync::IAssetAccessor::THeader>(),
      wmtsOptions,
      options);
}
