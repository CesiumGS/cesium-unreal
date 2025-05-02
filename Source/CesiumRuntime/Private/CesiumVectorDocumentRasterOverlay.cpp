// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumVectorDocumentRasterOverlay.h"

#include "CesiumCustomVersion.h"
#include "CesiumGeometry/QuadtreeTilingScheme.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "CesiumGeospatial/Projection.h"
#include "CesiumRasterOverlays/VectorDocumentRasterOverlay.h"
#include "CesiumVectorData/VectorRasterizerStyle.h"

#include "CesiumRuntime.h"

namespace {
CesiumVectorData::VectorStyle
unrealToNativeVectorStyle(const FCesiumVectorStyle& style) {
  return CesiumVectorData::VectorStyle{
      CesiumVectorData::LineStyle{
          CesiumVectorData::Color{
              style.LineStyle.Color.R,
              style.LineStyle.Color.G,
              style.LineStyle.Color.B,
              style.LineStyle.Color.A},
          (CesiumVectorData::ColorMode)style.LineStyle.ColorMode,
          style.LineStyle.Width,
          (CesiumVectorData::LineWidthMode)style.LineStyle.WidthMode},
      CesiumVectorData::PolygonStyle{
          CesiumVectorData::Color{
              style.PolygonStyle.Color.R,
              style.PolygonStyle.Color.G,
              style.PolygonStyle.Color.B,
              style.PolygonStyle.Color.A},
          (CesiumVectorData::ColorMode)style.PolygonStyle.ColorMode,
          style.PolygonStyle.Fill,
          style.PolygonStyle.Outline}};
}
} // namespace

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

  CesiumVectorData::VectorStyle style =
      unrealToNativeVectorStyle(this->DefaultStyle);

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
      unrealToNativeVectorStyle(this->DefaultStyle),
      callbackOpt,
      std::move(projection),
      options.ellipsoid,
      this->MipLevels};

  const CesiumGeospatial::Ellipsoid& ellipsoid = options.ellipsoid;

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
