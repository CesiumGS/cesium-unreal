
#include "CesiumArcGisMapServerRasterOverlay.h"
#include "Cesium3DTilesSelection/ArcGisMapServerRasterOverlay.h"
#include "CesiumRuntime.h"

std::unique_ptr<Cesium3DTilesSelection::RasterOverlay>
UCesiumArcGisMapServerRasterOverlay::CreateOverlay() {
  Cesium3DTilesSelection::ArcGisMapServerRasterOverlayOptions Options;
  if (MaximumLevel > MinimumLevel && bSpecifyZoomLevels) {
    Options.minimumLevel = MinimumLevel;
    Options.maximumLevel = MaximumLevel;
  }
  return std::make_unique<Cesium3DTilesSelection::ArcGisMapServerRasterOverlay>(
      TCHAR_TO_UTF8(*this->Url),
      // std::vector<CesiumAsync::IAssetAccessor::THeader>(),
      Options);
}
