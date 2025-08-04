// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumGeoJsonDocumentRasterOverlay.h"

#include "CesiumCustomVersion.h"
#include "CesiumGeometry/QuadtreeTilingScheme.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "CesiumGeospatial/Projection.h"
#include "CesiumRasterOverlays/GeoJsonDocumentRasterOverlay.h"
#include "CesiumVectorData/VectorStyle.h"

#include "CesiumRuntime.h"

namespace {
CesiumAsync::Future<std::shared_ptr<CesiumVectorData::GeoJsonDocument>>
wrapLoaderFuture(
    UCesiumGeoJsonDocumentRasterOverlay* pThis,
    CesiumAsync::Future<
        CesiumUtility::Result<CesiumVectorData::GeoJsonDocument>>&& future) {
  return std::move(future).thenInMainThread(
      [pThis](CesiumUtility::Result<CesiumVectorData::GeoJsonDocument>&&
                  documentResult)
          -> std::shared_ptr<CesiumVectorData::GeoJsonDocument> {
        if (documentResult.errors) {
          documentResult.errors.logError(
              spdlog::default_logger(),
              "Errors loading GeoJSON document: ");
          return nullptr;
        }

        std::shared_ptr<CesiumVectorData::GeoJsonDocument> pGeoJsonDocument =
            std::make_shared<CesiumVectorData::GeoJsonDocument>(
                std::move(*documentResult.value));

        if (pThis->OnDocumentLoaded.IsBound()) {
          pThis->OnDocumentLoaded.Execute(FCesiumGeoJsonDocument(
              std::shared_ptr<CesiumVectorData::GeoJsonDocument>(
                  pGeoJsonDocument)));
        }

        return pGeoJsonDocument;
      });
}
} // namespace

std::unique_ptr<CesiumRasterOverlays::RasterOverlay>
UCesiumGeoJsonDocumentRasterOverlay::CreateOverlay(
    const CesiumRasterOverlays::RasterOverlayOptions& options) {
  if (this->Source == ECesiumGeoJsonDocumentRasterOverlaySource::FromDocument &&
      !this->GeoJsonDocument.IsValid()) {
    // Don't create an overlay with an invalid document.
    return nullptr;
  }

  CesiumRasterOverlays::GeoJsonDocumentRasterOverlayOptions vectorOptions{
      this->DefaultStyle.toNative(),
      options.ellipsoid,
      this->MipLevels};

  if (this->Source ==
      ECesiumGeoJsonDocumentRasterOverlaySource::FromCesiumIon) {
    if (!IsValid(this->CesiumIonServer)) {
      this->CesiumIonServer = UCesiumIonServer::GetServerForNewObjects();
    }

    return std::make_unique<CesiumRasterOverlays::GeoJsonDocumentRasterOverlay>(
        TCHAR_TO_UTF8(*this->MaterialLayerKey),
        wrapLoaderFuture(
            this,
            CesiumVectorData::GeoJsonDocument::fromCesiumIonAsset(
                getAsyncSystem(),
                getAssetAccessor(),
                this->IonAssetID,
                TCHAR_TO_UTF8(*this->CesiumIonServer->DefaultIonAccessToken),
                std::string(TCHAR_TO_UTF8(*this->CesiumIonServer->ApiUrl)) +
                    "/")),
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
        wrapLoaderFuture(
            this,
            CesiumVectorData::GeoJsonDocument::fromUrl(
                getAsyncSystem(),
                getAssetAccessor(),
                TCHAR_TO_UTF8(*this->Url),
                std::move(headers))),
        vectorOptions,
        options);
  }

  if (this->OnDocumentLoaded.IsBound()) {
    this->OnDocumentLoaded.Execute(this->GeoJsonDocument);
  }

  return std::make_unique<CesiumRasterOverlays::GeoJsonDocumentRasterOverlay>(
      getAsyncSystem(),
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      this->GeoJsonDocument.GetDocument(),
      vectorOptions,
      options);
}
