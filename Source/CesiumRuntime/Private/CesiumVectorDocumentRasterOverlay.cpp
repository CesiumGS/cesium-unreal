// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumVectorDocumentRasterOverlay.h"

#include "CesiumCustomVersion.h"
#include "CesiumGeometry/QuadtreeTilingScheme.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "CesiumGeospatial/Projection.h"
#include "CesiumRasterOverlays/VectorDocumentRasterOverlay.h"

#include "CesiumRuntime.h"

std::unique_ptr<CesiumRasterOverlays::RasterOverlay>
UCesiumVectorDocumentRasterOverlay::CreateOverlay(
    const CesiumRasterOverlays::RasterOverlayOptions& options) {
  if (!this->VectorDocument.IsValid()) {
    // Don't create an overlay with an invalid document.
    return nullptr;
  }

  const CesiumGeospatial::Ellipsoid& ellipsoid = options.ellipsoid;

  CesiumGeospatial::Projection projection;
  if (this->Projection ==
      ECesiumVectorDocumentRasterOverlayProjection::Geographic) {
    projection = CesiumGeospatial::GeographicProjection(ellipsoid);
  } else {
    projection = CesiumGeospatial::WebMercatorProjection(ellipsoid);
  }

  return std::make_unique<CesiumRasterOverlays::VectorDocumentRasterOverlay>(
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      this->VectorDocument.GetDocument(),
      CesiumVectorData::Color{
          std::byte{this->Color.R},
          std::byte{this->Color.G},
          std::byte{this->Color.B},
          std::byte{this->Color.A}},
      projection,
      options);
}
