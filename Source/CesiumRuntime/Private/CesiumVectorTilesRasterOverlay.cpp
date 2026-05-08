// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumVectorTilesRasterOverlay.h"

#include "CesiumCustomVersion.h"
#include "CesiumGeometry/QuadtreeTilingScheme.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "CesiumGeospatial/Projection.h"
#include "CesiumRasterOverlays/GeoJsonDocumentRasterOverlay.h"
#include "CesiumVectorData/VectorStyle.h"
#include "Cesium3DTilesSelection/VectorTilesRasterOverlay.h"

#include "CesiumRuntime.h"

std::unique_ptr<CesiumRasterOverlays::RasterOverlay>
UCesiumVectorTilesRasterOverlay::CreateOverlay(
    const CesiumRasterOverlays::RasterOverlayOptions& options) {
  return std::make_unique<Cesium3DTilesSelection::VectorTilesRasterOverlay>(
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      TCHAR_TO_UTF8(*this->Url),
      this->DefaultStyle.toNative(),
      options);
}
