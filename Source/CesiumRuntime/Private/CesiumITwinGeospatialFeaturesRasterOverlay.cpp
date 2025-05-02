// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumITwinGeospatialFeaturesRasterOverlay.h"

#include "CesiumClientCommon/OAuth2PKCE.h"
#include "CesiumCustomVersion.h"
#include "CesiumGeometry/QuadtreeTilingScheme.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "CesiumGeospatial/Projection.h"
#include "CesiumITwinClient/AuthenticationToken.h"
#include "CesiumITwinClient/Connection.h"
#include "CesiumITwinClient/ITwinGeospatialFeaturesRasterOverlay.h"
#include "CesiumVectorData/VectorRasterizerStyle.h"

#include "CesiumRuntime.h"

std::unique_ptr<CesiumRasterOverlays::RasterOverlay>
UCesiumITwinGeospatialFeaturesRasterOverlay::CreateOverlay(
    const CesiumRasterOverlays::RasterOverlayOptions& options) {
  if (this->ITwinID.IsEmpty() || this->ITwinToken.IsEmpty() ||
      this->CollectionID.IsEmpty()) {
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
                          CesiumVectorData::VectorDocument>& doc,
                      const CesiumVectorData::VectorNode* pNode)
        -> std::optional<CesiumVectorData::VectorStyle> {
      FCesiumVectorStyle style;
      if (Callback.Execute(FCesiumVectorNode(doc, pNode), style)) {
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

  CesiumUtility::Result<CesiumITwinClient::AuthenticationToken> tokenResult =
      CesiumITwinClient::AuthenticationToken::parse(
          TCHAR_TO_UTF8(*this->ITwinToken));

  if (!tokenResult.value) {
    tokenResult.errors.logError(
        spdlog::default_logger(),
        "Invalid ITwinToken: ");
    return nullptr;
  }

  CesiumUtility::IntrusivePointer<CesiumITwinClient::Connection> pConnection;
  pConnection.emplace(
      getAsyncSystem(),
      getAssetAccessor(),
      *tokenResult.value,
      std::nullopt,
      CesiumClientCommon::OAuth2ClientOptions{});

  return std::make_unique<
      CesiumITwinClient::ITwinGeospatialFeaturesRasterOverlay>(
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      TCHAR_TO_UTF8(*this->ITwinID),
      TCHAR_TO_UTF8(*this->CollectionID),
      pConnection,
      vectorOptions,
      options);
}
