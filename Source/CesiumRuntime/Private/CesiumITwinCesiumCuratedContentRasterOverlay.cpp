// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumITwinCesiumCuratedContentRasterOverlay.h"
#include "Cesium3DTilesSelection/Tileset.h"
#include "CesiumActors.h"
#include "CesiumCustomVersion.h"
#include "CesiumIonServer.h"
#include "CesiumRasterOverlays/ITwinCesiumCuratedContentRasterOverlay.h"
#include "CesiumRuntime.h"
#include "CesiumRuntimeSettings.h"

#if WITH_EDITOR
#include "FileHelpers.h"
#endif

std::unique_ptr<CesiumRasterOverlays::RasterOverlay>
UCesiumITwinCesiumCuratedContentRasterOverlay::CreateOverlay(
    const CesiumRasterOverlays::RasterOverlayOptions& options) {
  if (this->AssetID <= 0) {
    // Don't create an overlay for an invalid asset ID.
    return nullptr;
  }

  return std::make_unique<
      CesiumRasterOverlays::ITwinCesiumCuratedContentRasterOverlay>(
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      this->AssetID,
      TCHAR_TO_UTF8(*this->ITwinAccessToken),
      options);
}
