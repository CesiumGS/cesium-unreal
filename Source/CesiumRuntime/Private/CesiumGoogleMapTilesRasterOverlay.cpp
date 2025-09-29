// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#include "CesiumGoogleMapTilesRasterOverlay.h"
#include "Cesium3DTilesSelection/Tileset.h"
#include "CesiumRasterOverlays/GoogleMapTilesRasterOverlay.h"

using namespace CesiumRasterOverlays;

namespace {

std::string getMapType(EGoogleMapTilesMapType mapType) {
  switch (mapType) {
  case EGoogleMapTilesMapType::Roadmap:
    return GoogleMapTilesMapType::roadmap;
  case EGoogleMapTilesMapType::Terrain:
    return GoogleMapTilesMapType::terrain;
  case EGoogleMapTilesMapType::Satellite:
  default:
    return GoogleMapTilesMapType::satellite;
  }
}

} // namespace

std::unique_ptr<CesiumRasterOverlays::RasterOverlay>
UCesiumGoogleMapTilesRasterOverlay::CreateOverlay(
    const CesiumRasterOverlays::RasterOverlayOptions& options) {
  return std::make_unique<CesiumRasterOverlays::GoogleMapTilesRasterOverlay>(
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      CesiumRasterOverlays::GoogleMapTilesNewSessionParameters{
          .key = TCHAR_TO_UTF8(*this->Key),
          .mapType = getMapType(this->MapType)},
      options);
}
