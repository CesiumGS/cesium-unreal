#include "CesiumMetadata.h"

TMap<FString, FCesiumMetadataValue>
UCesiumMetadataBlueprintFunctionLibrary::GetMetadataByFeatureID(
    FCesiumMetadata& Metadata,
    int64 FeatureID) {
  return {};
}

FCesiumMetadataProperty
UCesiumMetadataBlueprintFunctionLibrary::GetPropertyByName(
    UPARAM(ref) FCesiumMetadata& Metadata,
    const FString& name) {
  return FCesiumMetadataProperty{};
}
