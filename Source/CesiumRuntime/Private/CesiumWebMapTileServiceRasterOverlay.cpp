// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumWebMapTileServiceRasterOverlay.h"

#include "CesiumCustomVersion.h"
#include "CesiumGeometry/QuadtreeTilingScheme.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "CesiumGeospatial/Projection.h"
#include "CesiumRasterOverlays/WebMapTileServiceRasterOverlay.h"

#include "CesiumRuntime.h"

void UCesiumWebMapTileServiceRasterOverlay::Serialize(FArchive& Ar) {
  Super::Serialize(Ar);

  Ar.UsingCustomVersion(FCesiumCustomVersion::GUID);

  const int32 CesiumVersion = Ar.CustomVer(FCesiumCustomVersion::GUID);

  if (CesiumVersion < FCesiumCustomVersion::WebMapTileServiceProjectionAsEnum) {
    // In previous versions, the projection of the overlay was controlled by
    // boolean, rather than being explicitly specified by an enum.
    this->Projection =
        this->UseWebMercatorProjection_DEPRECATED
            ? ECesiumWebMapTileServiceRasterOverlayProjection::WebMercator
            : ECesiumWebMapTileServiceRasterOverlayProjection::Geographic;
  }
}

std::unique_ptr<CesiumRasterOverlays::RasterOverlay>
UCesiumWebMapTileServiceRasterOverlay::CreateOverlay(
    const CesiumRasterOverlays::RasterOverlayOptions& options) {
  if (this->BaseUrl.IsEmpty()) {
    // Don't create an overlay with an empty base URL.
    return nullptr;
  }

  CesiumRasterOverlays::WebMapTileServiceRasterOverlayOptions wmtsOptions;
  if (!Style.IsEmpty()) {
    wmtsOptions.style = TCHAR_TO_UTF8(*this->Style);
  }
  if (!Layer.IsEmpty()) {
    wmtsOptions.layer = TCHAR_TO_UTF8(*this->Layer);
  }
  if (!Format.IsEmpty()) {
    wmtsOptions.format = TCHAR_TO_UTF8(*this->Format);
  }
  if (!TileMatrixSetID.IsEmpty()) {
    wmtsOptions.tileMatrixSetID = TCHAR_TO_UTF8(*this->TileMatrixSetID);
  }

  if (bSpecifyZoomLevels && MaximumLevel > MinimumLevel) {
    wmtsOptions.minimumLevel = MinimumLevel;
    wmtsOptions.maximumLevel = MaximumLevel;
  }

  wmtsOptions.tileWidth = this->TileWidth;
  wmtsOptions.tileHeight = this->TileHeight;

  const CesiumGeospatial::Ellipsoid& ellipsoid = options.ellipsoid;

  if (this->Projection ==
      ECesiumWebMapTileServiceRasterOverlayProjection::Geographic) {
    wmtsOptions.projection = CesiumGeospatial::GeographicProjection(ellipsoid);
  } else {
    wmtsOptions.projection = CesiumGeospatial::WebMercatorProjection(ellipsoid);
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
            *wmtsOptions.projection,
            globeRectangle);
    wmtsOptions.coverageRectangle = coverageRectangle;
    wmtsOptions.tilingScheme = CesiumGeometry::QuadtreeTilingScheme(
        coverageRectangle,
        RootTilesX,
        RootTilesY);
  }

  if (bSpecifyTileMatrixSetLabels) {
    if (!TileMatrixSetLabels.IsEmpty()) {
      std::vector<std::string> labels;
      for (const auto& label : this->TileMatrixSetLabels) {
        labels.emplace_back(TCHAR_TO_UTF8(*label));
      }
      wmtsOptions.tileMatrixLabels = labels;
    }
  } else {
    if (!TileMatrixSetLabelPrefix.IsEmpty()) {
      std::vector<std::string> labels;
      for (size_t level = 0; level <= 25; ++level) {
        FString label(TileMatrixSetLabelPrefix);
        label.AppendInt(level);
        labels.emplace_back(TCHAR_TO_UTF8(*label));
      }
      wmtsOptions.tileMatrixLabels = labels;
    }
  }
  return std::make_unique<CesiumRasterOverlays::WebMapTileServiceRasterOverlay>(
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      TCHAR_TO_UTF8(*this->BaseUrl),
      std::vector<CesiumAsync::IAssetAccessor::THeader>(),
      wmtsOptions,
      options);
}
