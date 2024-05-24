// Copyright github.com/spr1ngd

#include "CesiumTileMapRasterOverlay.h"
#include "CesiumRasterOverlays/TileMapRasterOverlay.h"
#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumGeospatial/Projection.h>

std::unique_ptr<CesiumRasterOverlays::RasterOverlay> UCesiumTileMapRasterOverlay::CreateOverlay(const CesiumRasterOverlays::RasterOverlayOptions& options)
{
    if( this->Url.IsEmpty() )
    {
        return nullptr;
    }

    CesiumRasterOverlays::TileMapRasterOverlayOptions tmOptions;
    tmOptions.minimumLevel = MinimumLevel;
    tmOptions.maximumLevel = MaximumLevel;
    tmOptions.format = TCHAR_TO_UTF8(*this->Format);
    tmOptions.flipY = this->bFlipY;

    CesiumGeospatial::Projection projection;
    if (this->UseWebMercatorProjection)
    {
        projection = CesiumGeospatial::WebMercatorProjection();
        tmOptions.projection = projection;
    }
    else
    {
        projection = CesiumGeospatial::GeographicProjection();
        tmOptions.projection = projection;
    }

  if (bSpecifyTilingScheme)
  {
      CesiumGeospatial::GlobeRectangle globeRectangle =
          CesiumGeospatial::GlobeRectangle::fromDegrees(West, South, East, North);
      CesiumGeometry::Rectangle coverageRectangle =
          CesiumGeospatial::projectRectangleSimple(projection, globeRectangle);
      tmOptions.coverageRectangle = coverageRectangle;
      tmOptions.tilingScheme = CesiumGeometry::QuadtreeTilingScheme(
          coverageRectangle,
          RootTilesX,
          RootTilesY);
  }

    return std::make_unique<CesiumRasterOverlays::TileMapRasterOverlay>(
        TCHAR_TO_UTF8(*this->MaterialLayerKey),
        TCHAR_TO_UTF8(*this->Url),
        tmOptions,
        options
    );
}
