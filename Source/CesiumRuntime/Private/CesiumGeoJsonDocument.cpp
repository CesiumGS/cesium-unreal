#include "CesiumGeoJsonDocument.h"
#include "CesiumRuntime.h"

#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumUtility/Result.h>

#include <span>

bool UCesiumGeoJsonDocumentBlueprintLibrary::LoadGeoJsonFromString(
    const FString& InString,
    FCesiumGeoJsonDocument& OutVectorDocument) {
  const std::string str = TCHAR_TO_UTF8(*InString);
  std::span<const std::byte> bytes(
      reinterpret_cast<const std::byte*>(str.data()),
      str.size());
  CesiumUtility::Result<CesiumVectorData::GeoJsonDocument> documentResult =
      CesiumVectorData::GeoJsonDocument::fromGeoJson(bytes);

  if (!documentResult.errors.errors.empty()) {
    documentResult.errors.logError(
        spdlog::default_logger(),
        "Errors while loading GeoJSON from string");
  }

  if (!documentResult.errors.warnings.empty()) {
    documentResult.errors.logWarning(
        spdlog::default_logger(),
        "Warnings while loading GeoJSON from string");
  }

  if (documentResult.value) {
    OutVectorDocument = FCesiumGeoJsonDocument(
        std::make_shared<CesiumVectorData::GeoJsonDocument>(
            std::move(*documentResult.value)));
    return true;
  }

  return false;
}

FCesiumGeoJsonObject UCesiumGeoJsonDocumentBlueprintLibrary::GetRootObject(
    const FCesiumGeoJsonDocument& InVectorDocument) {
  if (!InVectorDocument._document) {
    return FCesiumGeoJsonObject();
  }

  return FCesiumGeoJsonObject(
      InVectorDocument._document,
      &InVectorDocument._document->rootObject);
}

UCesiumLoadVectorDocumentFromIonAsyncAction*
UCesiumLoadVectorDocumentFromIonAsyncAction::LoadFromIon(
    int64 AssetId,
    const FString& IonAccessToken,
    const FString& IonAssetEndpointUrl) {
  UCesiumLoadVectorDocumentFromIonAsyncAction* pAction =
      NewObject<UCesiumLoadVectorDocumentFromIonAsyncAction>();
  pAction->AssetId = AssetId;
  pAction->IonAccessToken = IonAccessToken;
  pAction->IonAssetEndpointUrl = IonAssetEndpointUrl;
  return pAction;
}

void UCesiumLoadVectorDocumentFromIonAsyncAction::Activate() {
  CesiumVectorData::GeoJsonDocument::fromCesiumIonAsset(
      getAsyncSystem(),
      getAssetAccessor(),
      this->AssetId,
      TCHAR_TO_UTF8(*this->IonAccessToken),
      TCHAR_TO_UTF8(*this->IonAssetEndpointUrl))
      .thenInMainThread(
          [Callback = this->OnLoadResult](
              CesiumUtility::Result<CesiumVectorData::GeoJsonDocument>&&
                  result) {
            if (result.errors.hasErrors()) {
              result.errors.logError(
                  spdlog::default_logger(),
                  "Errors loading GeoJSON:");
              result.errors.logWarning(
                  spdlog::default_logger(),
                  "Warnings loading GeoJSON:");
            }

            if (result.value) {
              Callback.Broadcast(
                  true,
                  FCesiumGeoJsonDocument(
                      std::make_shared<CesiumVectorData::GeoJsonDocument>(
                          std::move(*result.value))));
            } else {
              Callback.Broadcast(false, {});
            }
          });
}

UCesiumLoadVectorDocumentFromUrlAsyncAction*
UCesiumLoadVectorDocumentFromUrlAsyncAction::LoadFromUrl(
    const FString& Url,
    const TMap<FString, FString>& Headers) {
  UCesiumLoadVectorDocumentFromUrlAsyncAction* pAction =
      NewObject<UCesiumLoadVectorDocumentFromUrlAsyncAction>();
  pAction->Url = Url;
  pAction->Headers = Headers;
  return pAction;
}

void UCesiumLoadVectorDocumentFromUrlAsyncAction::Activate() {
  std::vector<CesiumAsync::IAssetAccessor::THeader> requestHeaders;
  requestHeaders.reserve(this->Headers.Num());

  for (const auto& [Key, Value] : this->Headers) {
    requestHeaders.emplace_back(CesiumAsync::IAssetAccessor::THeader{
        TCHAR_TO_UTF8(*Key),
        TCHAR_TO_UTF8(*Value)});
  }

  CesiumVectorData::GeoJsonDocument::fromUrl(
      getAsyncSystem(),
      getAssetAccessor(),
      TCHAR_TO_UTF8(*this->Url),
      std::move(requestHeaders))
      .thenInMainThread(
          [Callback = this->OnLoadResult](
              CesiumUtility::Result<CesiumVectorData::GeoJsonDocument>&&
                  result) {
            if (result.errors.hasErrors()) {
              result.errors.logError(
                  spdlog::default_logger(),
                  "Errors loading GeoJSON:");
              result.errors.logWarning(
                  spdlog::default_logger(),
                  "Warnings loading GeoJSON:");
            }

            if (result.value) {
              Callback.Broadcast(
                  true,
                  FCesiumGeoJsonDocument(
                      std::make_shared<CesiumVectorData::GeoJsonDocument>(
                          std::move(*result.value))));
            } else {
              Callback.Broadcast(false, {});
            }
          });
}
