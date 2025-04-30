// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumTileMapServiceRasterOverlay.h"
#include "CesiumRasterOverlays/TileMapServiceRasterOverlay.h"
#include "CesiumRuntime.h"

std::unique_ptr<CesiumRasterOverlays::RasterOverlay>
UCesiumTileMapServiceRasterOverlay::CreateOverlay(
    const CesiumRasterOverlays::RasterOverlayOptions& options) {
  if (this->Url.IsEmpty()) {
    // Don't create an overlay with an empty URL.
    return nullptr;
  }

  CesiumRasterOverlays::TileMapServiceRasterOverlayOptions tmsOptions;
  if (MaximumLevel > MinimumLevel && bSpecifyZoomLevels) {
    tmsOptions.minimumLevel = MinimumLevel;
    tmsOptions.maximumLevel = MaximumLevel;
  }

  std::vector<CesiumAsync::IAssetAccessor::THeader> headers;

  for (const auto& [Key, Value] : this->RequestHeaders) {
    headers.push_back(CesiumAsync::IAssetAccessor::THeader{
        TCHAR_TO_UTF8(*Key),
        TCHAR_TO_UTF8(*Value)});
  }

  return std::make_unique<CesiumRasterOverlays::TileMapServiceRasterOverlay>(
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      TCHAR_TO_UTF8(*this->Url),
      headers,
      tmsOptions,
      options);
}

bool UCesiumTileMapServiceRasterOverlay::IsReadyForFinishDestroy() {
  this->SetUrl(this->Url);
  return Super::IsReadyForFinishDestroy();
}