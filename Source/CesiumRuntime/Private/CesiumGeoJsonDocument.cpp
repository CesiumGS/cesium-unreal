#include "CesiumGeoJsonDocument.h"
#include "CesiumIonServer.h"
#include "CesiumRuntime.h"

#include <CesiumAsync/IAssetAccessor.h>
#include <CesiumUtility/Result.h>

#include <span>

FCesiumGeoJsonDocument::FCesiumGeoJsonDocument() : _pDocument(nullptr) {}

FCesiumGeoJsonDocument::FCesiumGeoJsonDocument(
    std::shared_ptr<CesiumVectorData::GeoJsonDocument>&& document)
    : _pDocument(std::move(document)) {}

bool FCesiumGeoJsonDocument::IsValid() const {
  return this->_pDocument != nullptr;
}

const std::shared_ptr<CesiumVectorData::GeoJsonDocument>&
FCesiumGeoJsonDocument::GetDocument() const {
  return this->_pDocument;
}

bool UCesiumGeoJsonDocumentBlueprintLibrary::LoadGeoJsonFromString(
    const FString& InString,
    FCesiumGeoJsonDocument& OutGeoJsonDocument) {
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
    OutGeoJsonDocument = FCesiumGeoJsonDocument(
        std::make_shared<CesiumVectorData::GeoJsonDocument>(
            std::move(*documentResult.value)));
    return true;
  }

  return false;
}

FCesiumGeoJsonObject UCesiumGeoJsonDocumentBlueprintLibrary::GetRootObject(
    const FCesiumGeoJsonDocument& InGeoJsonDocument) {
  if (!InGeoJsonDocument._pDocument) {
    return FCesiumGeoJsonObject();
  }

  return FCesiumGeoJsonObject(
      InGeoJsonDocument._pDocument,
      &InGeoJsonDocument._pDocument->rootObject);
}

UCesiumLoadGeoJsonDocumentFromIonAsyncAction*
UCesiumLoadGeoJsonDocumentFromIonAsyncAction::LoadFromIon(
    int64 AssetId,
    const FString& IonAccessToken,
    const UCesiumIonServer* CesiumIonServer) {
  UCesiumLoadGeoJsonDocumentFromIonAsyncAction* pAction =
      NewObject<UCesiumLoadGeoJsonDocumentFromIonAsyncAction>();
  pAction->AssetId = AssetId;
  pAction->IonAccessToken = IonAccessToken;
  pAction->CesiumIonServer = CesiumIonServer;
  return pAction;
}

void UCesiumLoadGeoJsonDocumentFromIonAsyncAction::Activate() {
  // Make sure we have a valid Cesium ion server.
  if (!IsValid(this->CesiumIonServer)) {
    this->CesiumIonServer = UCesiumIonServer::GetServerForNewObjects();
  }

  const std::string token(
      this->IonAccessToken.IsEmpty()
          ? TCHAR_TO_UTF8(*this->CesiumIonServer->DefaultIonAccessToken)
          : TCHAR_TO_UTF8(*this->IonAccessToken));
  CesiumVectorData::GeoJsonDocument::fromCesiumIonAsset(
      getAsyncSystem(),
      getAssetAccessor(),
      this->AssetId,
      token,
      std::string(TCHAR_TO_UTF8(*this->CesiumIonServer->ApiUrl)) + "/")
      .thenInMainThread(
          [Callback = this->OnLoadResult](
              CesiumUtility::Result<CesiumVectorData::GeoJsonDocument>&&
                  result) {
            if (result.errors.hasErrors()) {
              result.errors.logError(
                  spdlog::default_logger(),
                  "Errors loading GeoJSON");
              result.errors.logWarning(
                  spdlog::default_logger(),
                  "Warnings loading GeoJSON");
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

UCesiumLoadGeoJsonDocumentFromUrlAsyncAction*
UCesiumLoadGeoJsonDocumentFromUrlAsyncAction::LoadFromUrl(
    const FString& Url,
    const TMap<FString, FString>& Headers) {
  UCesiumLoadGeoJsonDocumentFromUrlAsyncAction* pAction =
      NewObject<UCesiumLoadGeoJsonDocumentFromUrlAsyncAction>();
  pAction->Url = Url;
  pAction->Headers = Headers;
  return pAction;
}

void UCesiumLoadGeoJsonDocumentFromUrlAsyncAction::Activate() {
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
                  "Errors loading GeoJSON");
              result.errors.logWarning(
                  spdlog::default_logger(),
                  "Warnings loading GeoJSON");
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
