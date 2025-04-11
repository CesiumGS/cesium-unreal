#include "CesiumVectorDocument.h"
#include "CesiumRuntime.h"

#include "CesiumUtility/Result.h"

#include <span>

bool UCesiumVectorDocumentBlueprintLibrary::LoadGeoJsonFromString(
    const FString& InString,
    FCesiumVectorDocument& OutVectorDocument) {
  const std::string str = TCHAR_TO_UTF8(*InString);
  std::span<const std::byte> bytes(
      reinterpret_cast<const std::byte*>(str.data()),
      str.size());
  CesiumUtility::Result<CesiumVectorData::VectorDocument> documentResult =
      CesiumVectorData::VectorDocument::fromGeoJson(bytes);

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
    OutVectorDocument = FCesiumVectorDocument(std::move(*documentResult.value));
    return true;
  }

  return false;
}

FCesiumVectorNode UCesiumVectorDocumentBlueprintLibrary::GetRootNode(
    const FCesiumVectorDocument& InVectorDocument) {
  if (!InVectorDocument._document.IsValid()) {
    return FCesiumVectorNode();
  }
  return FCesiumVectorNode(
      InVectorDocument._document,
      &InVectorDocument._document->getRootNode());
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
  CesiumVectorData::VectorDocument::fromCesiumIonAsset(
      getAsyncSystem(),
      getAssetAccessor(),
      this->AssetId,
      TCHAR_TO_UTF8(*this->IonAccessToken),
      TCHAR_TO_UTF8(*this->IonAssetEndpointUrl))
      .thenInMainThread(
          [Callback = this->OnLoadResult](
              CesiumUtility::Result<CesiumVectorData::VectorDocument>&&
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
                  FCesiumVectorDocument(MoveTemp(*result.value)));
            } else {
              Callback.Broadcast(false, {});
            }
          });
}
