// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumWebMapServiceRasterOverlay.h"
#include "Algo/Transform.h"
#include "Cesium3DTilesSelection/WebMapServiceRasterOverlay.h"
#include "CesiumRuntime.h"

std::unique_ptr<Cesium3DTilesSelection::RasterOverlay>
UCesiumWebMapServiceRasterOverlay::CreateOverlay(
    const Cesium3DTilesSelection::RasterOverlayOptions& options) {

  Cesium3DTilesSelection::WebMapServiceRasterOverlayOptions wmsOptions;
  if (MaximumLevel > MinimumLevel && bSpecifyZoomLevels) {
    wmsOptions.minimumLevel = MinimumLevel;
    wmsOptions.maximumLevel = MaximumLevel;
  }
  wmsOptions.layers = TCHAR_TO_UTF8(*Layers);
  wmsOptions.tileWidth = TileWidth;
  wmsOptions.tileHeight = TileHeight;
  return std::make_unique<Cesium3DTilesSelection::WebMapServiceRasterOverlay>(
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      TCHAR_TO_UTF8(*this->BaseUrl),
      std::vector<CesiumAsync::IAssetAccessor::THeader>(),
      wmsOptions,
      options);
}

#if WITH_EDITOR

void UCesiumWebMapServiceRasterOverlay::PostEditChangeProperty(
    FPropertyChangedEvent& PropertyChangedEvent) {

  if (PropertyChangedEvent.Property &&
      (PropertyChangedEvent.Property->GetFName() ==
       GET_MEMBER_NAME_CHECKED(UCesiumWebMapServiceRasterOverlay, Layers))) {
    TArray<FString> OutArray;
    Layers.ParseIntoArray(OutArray, TEXT(","));
    TArray<FString> EncodedArray;

    Algo::Transform(OutArray, EncodedArray, [](auto& Layer) {
      return Layer.Replace(TEXT(" "), TEXT("%20"));
    });
    Layers = FString::Join(EncodedArray, TEXT(","));
  }

  Super::PostEditChangeProperty(PropertyChangedEvent);
}

#endif
