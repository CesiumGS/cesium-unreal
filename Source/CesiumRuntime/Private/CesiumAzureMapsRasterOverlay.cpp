// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#include "CesiumAzureMapsRasterOverlay.h"
#include "Cesium3DTilesSelection/Tileset.h"
#include "CesiumRasterOverlays/AzureMapsRasterOverlay.h"

using namespace CesiumRasterOverlays;

namespace {
std::string getTilesetId(EAzureMapsTilesetId tilesetId) {
  switch (tilesetId) {
  case EAzureMapsTilesetId::BaseDarkGrey:
    return AzureMapsTilesetId::baseDarkGrey;
  case EAzureMapsTilesetId::BaseLabelsRoad:
    return AzureMapsTilesetId::baseLabelsRoad;
  case EAzureMapsTilesetId::BaseLabelsDarkGrey:
    return AzureMapsTilesetId::baseLabelsDarkGrey;
  case EAzureMapsTilesetId::Imagery:
    return AzureMapsTilesetId::imagery;
  case EAzureMapsTilesetId::Terra:
    return AzureMapsTilesetId::terra;
  case EAzureMapsTilesetId::BaseRoad:
  default:
    return AzureMapsTilesetId::baseRoad;
  }
}
} // namespace

std::unique_ptr<CesiumRasterOverlays::RasterOverlay>
UCesiumAzureMapsRasterOverlay::CreateOverlay(
    const CesiumRasterOverlays::RasterOverlayOptions& options) {
  if (this->Key.IsEmpty()) {
    // We must have a key to create this overlay.
    return nullptr;
  }

  return std::make_unique<AzureMapsRasterOverlay>(
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      AzureMapsSessionParameters{
          .key = TCHAR_TO_UTF8(*this->Key),
          .apiVersion = TCHAR_TO_UTF8(*this->ApiVersion),
          .tilesetId = getTilesetId(this->TilesetId),
          .language = TCHAR_TO_UTF8(*this->Language),
          .view = TCHAR_TO_UTF8(*this->View),
      },
      options);
}
