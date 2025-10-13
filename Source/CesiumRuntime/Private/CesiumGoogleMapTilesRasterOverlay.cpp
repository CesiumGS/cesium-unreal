// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#include "CesiumGoogleMapTilesRasterOverlay.h"
#include "Cesium3DTilesSelection/Tileset.h"
#include "CesiumJsonReader/JsonObjectJsonHandler.h"
#include "CesiumJsonReader/JsonReader.h"
#include "CesiumRasterOverlays/GoogleMapTilesRasterOverlay.h"

using namespace CesiumJsonReader;
using namespace CesiumRasterOverlays;
using namespace CesiumUtility;

namespace {

std::string getMapType(EGoogleMapTilesMapType mapType) {
  switch (mapType) {
  case EGoogleMapTilesMapType::Roadmap:
    return GoogleMapTilesMapType::roadmap;
  case EGoogleMapTilesMapType::Terrain:
    return GoogleMapTilesMapType::terrain;
  case EGoogleMapTilesMapType::Satellite:
  default:
    return GoogleMapTilesMapType::satellite;
  }
}

std::string getScale(EGoogleMapTilesScale scale) {
  switch (scale) {
  case EGoogleMapTilesScale::ScaleFactor4x:
    return GoogleMapTilesScale::scaleFactor4x;
  case EGoogleMapTilesScale::ScaleFactor2x:
    return GoogleMapTilesScale::scaleFactor2x;
  case EGoogleMapTilesScale::ScaleFactor1x:
  default:
    return GoogleMapTilesScale::scaleFactor1x;
  }
}

std::optional<std::vector<std::string>> getLayerTypes(
    const TArray<EGoogleMapTilesLayerType>& layerTypes,
    EGoogleMapTilesMapType mapType) {
  std::vector<std::string> result;

  bool hasRoadmap = false;

  if (!layerTypes.IsEmpty()) {
    result.reserve(layerTypes.Num());

    for (EGoogleMapTilesLayerType layerType : layerTypes) {
      switch (layerType) {
      case EGoogleMapTilesLayerType::Roadmap:
        hasRoadmap = true;
        result.emplace_back(GoogleMapTilesLayerType::layerRoadmap);
        break;
      case EGoogleMapTilesLayerType::Streetview:
        result.emplace_back(GoogleMapTilesLayerType::layerStreetview);
        break;
      case EGoogleMapTilesLayerType::Traffic:
        result.emplace_back(GoogleMapTilesLayerType::layerTraffic);
        break;
      }
    }
  }

  if (mapType == EGoogleMapTilesMapType::Terrain && !hasRoadmap) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "When the MapType is set to Terrain, LayerTypes must contain Roadmap."));
  }

  return result;
}

JsonValue::Array getStyles(const TArray<FString>& styles) {
  JsonValue::Array result;
  result.reserve(styles.Num());

  JsonObjectJsonHandler handler{};

  for (int32 i = 0; i < styles.Num(); ++i) {
    const FString& style = styles[i];
    std::string styleUtf8 = TCHAR_TO_UTF8(*style);
    ReadJsonResult<JsonValue> response = JsonReader::readJson(
        std::span<const std::byte>(
            reinterpret_cast<const std::byte*>(styleUtf8.data()),
            styleUtf8.size()),
        handler);

    ErrorList errorList;
    errorList.errors = std::move(response.errors);
    errorList.warnings = std::move(response.warnings);
    errorList.log(
        spdlog::default_logger(),
        fmt::format("Problems parsing JSON in element {} of Styles:", i));

    if (response.value) {
      result.emplace_back(std::move(*response.value));
    }
  }

  return result;
}

} // namespace

std::unique_ptr<CesiumRasterOverlays::RasterOverlay>
UCesiumGoogleMapTilesRasterOverlay::CreateOverlay(
    const CesiumRasterOverlays::RasterOverlayOptions& options) {
  if (this->Key.IsEmpty()) {
    // We must have a key to create this overlay.
    return nullptr;
  }

  return std::make_unique<CesiumRasterOverlays::GoogleMapTilesRasterOverlay>(
      TCHAR_TO_UTF8(*this->MaterialLayerKey),
      CesiumRasterOverlays::GoogleMapTilesNewSessionParameters{
          .key = TCHAR_TO_UTF8(*this->Key),
          .mapType = getMapType(this->MapType),
          .language = TCHAR_TO_UTF8(*this->Language),
          .region = TCHAR_TO_UTF8(*this->Region),
          .scale = getScale(this->Scale),
          .highDpi = this->HighDPI,
          .layerTypes = getLayerTypes(this->LayerTypes, this->MapType),
          .styles = getStyles(this->Styles),
          .overlay = this->Overlay},
      options);
}
