// Copyright github.com/spr1ngd

#include "CesiumTileMapRasterOverlay.h"
#include "CesiumRasterOverlays/TileMapRasterOverlay.h"
#include <CesiumGeometry/QuadtreeTilingScheme.h>
#include <CesiumGeospatial/Projection.h>

void UCesiumTileMapRasterOverlay::SetRequestHeaders(TArray<FString> Keys, TArray<FString> Values)
{
    if( Keys.Num() != Values.Num() ) {
        UE_LOG(LogTemp, Error, TEXT("Number of Request Header's keys must be equal with values."));
        return;
    }

    this->headers = {};
    const int32 Num = Keys.Num();
    for( int32 idx = 0; idx < Num; idx++ )
    {
        this->headers.emplace_back(TCHAR_TO_UTF8(*Keys[idx]), TCHAR_TO_UTF8(*Values[idx]));
    }

#if WITH_EDITOR

    FString DebugString;
    DebugString += "{\r\n";
    for (CesiumAsync::IAssetAccessor::THeader Header : headers)
    {
        DebugString += FString::Printf(TEXT("%s : %s,\r\n"), *FString(Header.first.c_str()), *FString(Header.second.c_str()));
    }
    DebugString += "}\r\n";
    UE_LOG(LogTemp, Log, TEXT("%s"), *DebugString);

#endif
}

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
    tmOptions.tileMapSrc = this->TileMapSource.GetIntValue();

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
        headers,
        tmOptions,
        options
    );
}
