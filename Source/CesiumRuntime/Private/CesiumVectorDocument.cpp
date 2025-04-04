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
  return FCesiumVectorNode(InVectorDocument._document.getRootNode());
}
