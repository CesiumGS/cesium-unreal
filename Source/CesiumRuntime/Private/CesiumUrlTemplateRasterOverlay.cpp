// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumUrlTemplateRasterOverlay.h"

#include "CesiumCustomVersion.h"
#include "CesiumGeometry/QuadtreeTilingScheme.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "CesiumGeospatial/Projection.h"
#include "CesiumRasterOverlays/UrlTemplateRasterOverlay.h"

#include "CesiumRuntime.h"

std::unique_ptr<CesiumRasterOverlays::RasterOverlay>
UCesiumUrlTemplateRasterOverlay::CreateOverlay(
    const CesiumRasterOverlays::RasterOverlayOptions& options) {
  if (this->TemplateUrl.IsEmpty()) {
    // Don't create an overlay with an empty base URL.
    return nullptr;
  }

  CesiumRasterOverlays::UrlTemplateRasterOverlayOptions urlTemplateOptions;

  urlTemplateOptions.minimumLevel = MinimumLevel;
  urlTemplateOptions.maximumLevel = MaximumLevel;

  urlTemplateOptions.tileWidth = this->TileWidth;
  urlTemplateOptions.tileHeight = this->TileHeight;

  const CesiumGeospatial::Ellipsoid& ellipsoid = options.ellipsoid;

  if (this->Projection ==
      ECesiumUrlTemplateRasterOverlayProjection::Geographic) {
    urlTemplateOptions.projection =
        CesiumGeospatial::GeographicProjection(ellipsoid);
  } else {
    urlTemplateOptions.projection =
        CesiumGeospatial::WebMercatorProjection(ellipsoid);
  }

  if (bSpecifyTilingScheme) {
    CesiumGeospatial::GlobeRectangle globeRectangle =
        CesiumGeospatial::GlobeRectangle::fromDegrees(
            RectangleWest,
            RectangleSouth,
            RectangleEast,
            RectangleNorth);
    CesiumGeometry::Rectangle coverageRectangle =
        CesiumGeospatial::projectRectangleSimple(
            *urlTemplateOptions.projection,
            globeRectangle);
    urlTemplateOptions.coverageRectangle = coverageRectangle;
    urlTemplateOptions.tilingScheme = CesiumGeometry::QuadtreeTilingScheme(
        coverageRectangle,
        RootTilesX,
        RootTilesY);
  }

  std::vector<CesiumAsync::IAssetAccessor::THeader> headers;

  for (const auto& [Key, Value] : this->RequestHeaders) {
    headers.push_back(CesiumAsync::IAssetAccessor::THeader{
        TCHAR_TO_UTF8(*Key),
        TCHAR_TO_UTF8(*Value)});
  }

  return std::make_unique<CesiumRasterOverlays::UrlTemplateRasterOverlay>(
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      TCHAR_TO_UTF8(*this->TemplateUrl),
      headers,
      urlTemplateOptions,
      options);
}
