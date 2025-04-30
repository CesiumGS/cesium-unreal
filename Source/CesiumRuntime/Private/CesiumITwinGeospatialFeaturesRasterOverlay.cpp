// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumITwinGeospatialFeaturesRasterOverlay.h"

#include "CesiumCustomVersion.h"
#include "CesiumGeometry/QuadtreeTilingScheme.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "CesiumGeospatial/Projection.h"
#include "CesiumITwinClient/ITwinGeospatialFeaturesRasterOverlay.h"
#include "CesiumVectorData/VectorRasterizerStyle.h"

#include "CesiumRuntime.h"

std::unique_ptr<CesiumRasterOverlays::RasterOverlay>
UCesiumITwinGeospatialFeaturesRasterOverlay::CreateOverlay(
    const CesiumRasterOverlays::RasterOverlayOptions& options) {
  if (this->ITwinID.IsEmpty() || this->ITwinToken.IsEmpty() || this->CollectionID.IsEmpty()) {
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

  CesiumVectorData::VectorRasterizerStyle style{
      CesiumVectorData::Color{
          std::byte{this->Color.R},
          std::byte{this->Color.G},
          std::byte{this->Color.B},
          std::byte{this->Color.A}},
      this->LineWidth,
      (CesiumVectorData::VectorLineWidthMode)this->LineWidthMode};

  return std::make_unique<CesiumITwinClient::>(
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      this->VectorDocument.GetDocument(),
      style,
      projection,
      ellipsoid,
      (uint32_t)this->MipLevels,
      options);
}
