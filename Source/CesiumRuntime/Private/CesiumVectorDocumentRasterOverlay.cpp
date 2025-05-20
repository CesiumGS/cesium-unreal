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

  CesiumGeospatial::Projection projection;
  if (this->Projection ==
      ECesiumVectorDocumentRasterOverlayProjection::Geographic) {
    projection = CesiumGeospatial::GeographicProjection(options.ellipsoid);
  } else {
    projection = CesiumGeospatial::WebMercatorProjection(options.ellipsoid);
  }

  std::optional<CesiumRasterOverlays::VectorDocumentRasterOverlayStyleCallback>
      callbackOpt = std::nullopt;

  if (this->StyleCallback.IsBound()) {
    callbackOpt = [Callback = this->StyleCallback](
                      const CesiumUtility::IntrusivePointer<
                          CesiumVectorData::GeoJsonDocument>& doc,
                      const CesiumVectorData::GeoJsonObject* pNode)
        -> std::optional<CesiumVectorData::VectorStyle> {
      FCesiumVectorStyle style;
      if (Callback.Execute(FCesiumGeoJsonObject(doc, pNode), style)) {
        return style.toNative();
      }

      return std::nullopt;
    };
  }

  CesiumRasterOverlays::VectorDocumentRasterOverlayOptions vectorOptions{
      this->DefaultStyle.toNative(),
      callbackOpt,
      std::move(projection),
      options.ellipsoid,
      this->MipLevels};

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
        vectorOptions,
        options);
  }

  return std::make_unique<CesiumRasterOverlays::VectorDocumentRasterOverlay>(
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      this->VectorDocument.GetDocument(),
      vectorOptions,
      options);
}
