#include "CesiumGeoJsonDocument.h"
#include "CesiumRuntime.h"

#include "CesiumUtility/Result.h"

#include <span>

bool UCesiumVectorDocumentBlueprintLibrary::LoadGeoJsonFromString(
    const FString& InString,
    FCesiumGeoJsonDocument& OutVectorDocument) {
  const std::string str = TCHAR_TO_UTF8(*InString);
  std::span<const std::byte> bytes(
      reinterpret_cast<const std::byte*>(str.data()),
      str.size());
  CesiumUtility::Result<
      CesiumUtility::IntrusivePointer<CesiumVectorData::GeoJsonDocument>>
      documentResult = CesiumVectorData::GeoJsonDocument::fromGeoJson(bytes);

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

  if (documentResult.pValue) {
    OutVectorDocument =
        FCesiumGeoJsonDocument(std::move(documentResult.pValue));
    return true;
  }

  return false;
}

FCesiumGeoJsonObject UCesiumVectorDocumentBlueprintLibrary::GetRootNode(
    const FCesiumGeoJsonDocument& InVectorDocument) {
  if (!InVectorDocument._document) {
    return FCesiumGeoJsonObject();
  }

  return FCesiumGeoJsonObject(
      InVectorDocument._document,
      &InVectorDocument._document->getRootObject());
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
              CesiumUtility::Result<CesiumUtility::IntrusivePointer<
                  CesiumVectorData::GeoJsonDocument>>&& result) {
            if (result.errors.hasErrors()) {
              result.errors.logError(
                  spdlog::default_logger(),
                  "Errors loading GeoJSON:");
              result.errors.logWarning(
                  spdlog::default_logger(),
                  "Warnings loading GeoJSON:");
            }

            if (result.pValue) {
              Callback.Broadcast(
                  true,
                  FCesiumGeoJsonDocument(MoveTemp(result.pValue)));
            } else {
              Callback.Broadcast(false, {});
            }
          });
}
