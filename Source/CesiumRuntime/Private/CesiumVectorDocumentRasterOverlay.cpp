// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumVectorDocumentRasterOverlay.h"

#include "CesiumCustomVersion.h"
#include "CesiumGeometry/QuadtreeTilingScheme.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "CesiumGeospatial/Projection.h"
#include "CesiumRasterOverlays/VectorDocumentRasterOverlay.h"
#include "CesiumVectorData/VectorRasterizerStyle.h"

#include "CesiumRuntime.h"

std::unique_ptr<CesiumRasterOverlays::RasterOverlay>
UCesiumVectorDocumentRasterOverlay::CreateOverlay(
    const CesiumRasterOverlays::RasterOverlayOptions& options) {
  if (this->Source == ECesiumVectorDocumentRasterOverlaySource::FromDocument &&
      !this->VectorDocument.IsValid()) {
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

  const CesiumVectorData::Color color{
      std::byte{this->Color.R},
      std::byte{this->Color.G},
      std::byte{this->Color.B},
      std::byte{this->Color.A}};
  CesiumVectorData::VectorStyle style{
      CesiumVectorData::LineStyle{
          color,
          CesiumVectorData::ColorMode::Normal,
          this->LineWidth,
          (CesiumVectorData::LineWidthMode)this->LineWidthMode},
      CesiumVectorData::PolygonStyle{
          color,
          CesiumVectorData::ColorMode::Normal,
          true,
          false}};

  if (this->Source == ECesiumVectorDocumentRasterOverlaySource::FromCesiumIon) {
    if (!IsValid(this->CesiumIonServer)) {
      this->CesiumIonServer = UCesiumIonServer::GetServerForNewObjects();
    }

    return std::make_unique<CesiumRasterOverlays::VectorDocumentRasterOverlay>(
        TCHAR_TO_UTF8(*this->MaterialLayerKey),
        CesiumRasterOverlays::IonVectorDocumentRasterOverlaySource{
            this->IonAssetID,
            TCHAR_TO_UTF8(*this->CesiumIonServer->DefaultIonAccessToken),
            TCHAR_TO_UTF8(*this->CesiumIonServer->ApiUrl)},
        style,
        projection,
        ellipsoid,
        (uint32_t)this->MipLevels,
        options);
  }

  return std::make_unique<CesiumRasterOverlays::VectorDocumentRasterOverlay>(
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      this->VectorDocument.GetDocument(),
      style,
      projection,
      ellipsoid,
      (uint32_t)this->MipLevels,
      options);
}
