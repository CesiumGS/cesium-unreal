// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumGeoJsonDocumentRasterOverlay.h"

#include "CesiumCustomVersion.h"
#include "CesiumGeometry/QuadtreeTilingScheme.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "CesiumGeospatial/Projection.h"
#include "CesiumRasterOverlays/GeoJsonDocumentRasterOverlay.h"
#include "CesiumVectorData/VectorStyle.h"

#include "CesiumRuntime.h"

std::unique_ptr<CesiumRasterOverlays::RasterOverlay>
UCesiumGeoJsonDocumentRasterOverlay::CreateOverlay(
    const CesiumRasterOverlays::RasterOverlayOptions& options) {
  if (this->Source == ECesiumGeoJsonDocumentRasterOverlaySource::FromDocument &&
      !this->GeoJsonDocument.IsValid()) {
    // Don't create an overlay with an invalid document.
    return nullptr;
  }

  CesiumGeospatial::Projection projection;
  if (this->Projection ==
      ECesiumGeoJsonDocumentRasterOverlayProjection::Geographic) {
    projection = CesiumGeospatial::GeographicProjection(options.ellipsoid);
  } else {
    projection = CesiumGeospatial::WebMercatorProjection(options.ellipsoid);
  }

  std::optional<CesiumRasterOverlays::GeoJsonDocumentRasterOverlayStyleCallback>
      callbackOpt = std::nullopt;

  if (this->StyleCallback.IsBound()) {
    callbackOpt =
        [Callback = this->StyleCallback](
            const std::shared_ptr<CesiumVectorData::GeoJsonDocument>& doc,
            const CesiumVectorData::GeoJsonObject* pNode)
        -> std::optional<CesiumVectorData::VectorStyle> {
      FCesiumVectorStyle style;
      if (Callback.Execute(FCesiumGeoJsonObject(doc, pNode), style)) {
        return style.toNative();
      }

      return std::nullopt;
    };
  }

  CesiumRasterOverlays::GeoJsonDocumentRasterOverlayOptions vectorOptions{
      this->DefaultStyle.toNative(),
      callbackOpt,
      std::move(projection),
      options.ellipsoid,
      this->MipLevels};

  if (this->Source ==
      ECesiumGeoJsonDocumentRasterOverlaySource::FromCesiumIon) {
    if (!IsValid(this->CesiumIonServer)) {
      this->CesiumIonServer = UCesiumIonServer::GetServerForNewObjects();
    }

    return std::make_unique<CesiumRasterOverlays::GeoJsonDocumentRasterOverlay>(
        TCHAR_TO_UTF8(*this->MaterialLayerKey),
        CesiumRasterOverlays::IonGeoJsonDocumentRasterOverlaySource{
            this->IonAssetID,
            TCHAR_TO_UTF8(*this->CesiumIonServer->DefaultIonAccessToken),
            TCHAR_TO_UTF8(*this->CesiumIonServer->ApiUrl)},
        vectorOptions,
        options);
  } else if (
      this->Source == ECesiumGeoJsonDocumentRasterOverlaySource::FromUrl) {
    std::vector<CesiumAsync::IAssetAccessor::THeader> headers;
    headers.reserve(this->RequestHeaders.Num());

    for (auto& [k, v] : this->RequestHeaders) {
      headers.push_back({TCHAR_TO_UTF8(*k), TCHAR_TO_UTF8(*v)});
    }

    return std::make_unique<CesiumRasterOverlays::GeoJsonDocumentRasterOverlay>(
        TCHAR_TO_UTF8(*this->MaterialLayerKey),
        CesiumRasterOverlays::UrlGeoJsonDocumentRasterOverlaySource{
            TCHAR_TO_UTF8(*this->Url),
            std::move(headers)},
        vectorOptions,
        options);
  }

  return std::make_unique<CesiumRasterOverlays::GeoJsonDocumentRasterOverlay>(
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      this->GeoJsonDocument.GetDocument(),
      vectorOptions,
      options);
}
