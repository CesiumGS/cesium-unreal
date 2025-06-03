// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumWebMapServiceRasterOverlay.h"
#include "Algo/Transform.h"
#include "CesiumRasterOverlays/WebMapServiceRasterOverlay.h"
#include "CesiumRuntime.h"

std::unique_ptr<CesiumRasterOverlays::RasterOverlay>
UCesiumWebMapServiceRasterOverlay::CreateOverlay(
    const CesiumRasterOverlays::RasterOverlayOptions& options) {
  if (this->BaseUrl.IsEmpty()) {
    // Don't create an overlay with an empty base URL.
    return nullptr;
  }

  CesiumRasterOverlays::WebMapServiceRasterOverlayOptions wmsOptions;
  if (MaximumLevel > MinimumLevel) {
    wmsOptions.minimumLevel = MinimumLevel;
    wmsOptions.maximumLevel = MaximumLevel;
  }
  wmsOptions.layers = TCHAR_TO_UTF8(*Layers);
  wmsOptions.tileWidth = TileWidth;
  wmsOptions.tileHeight = TileHeight;

  std::vector<CesiumAsync::IAssetAccessor::THeader> headers;

  for (const auto& [Key, Value] : this->RequestHeaders) {
    headers.push_back(CesiumAsync::IAssetAccessor::THeader{
        TCHAR_TO_UTF8(*Key),
        TCHAR_TO_UTF8(*Value)});
  }

  return std::make_unique<CesiumRasterOverlays::WebMapServiceRasterOverlay>(
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      TCHAR_TO_UTF8(*this->BaseUrl),
      headers,
      wmsOptions,
      options);
}

bool UCesiumWebMapServiceRasterOverlay::IsReadyForFinishDestroy() {
  this->SetUrl(this->BaseUrl);
  return Super::IsReadyForFinishDestroy();
}