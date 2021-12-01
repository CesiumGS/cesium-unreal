#include "CesiumMetadataUtilityBlueprintLibrary.h"
#include "CesiumFeatureIDTexture.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumVertexMetadata.h"

FCesiumMetadataPrimitive
UCesiumMetadataUtilityBlueprintLibrary::GetPrimitiveMetadata(
    const UPrimitiveComponent* component) {
  const UCesiumGltfPrimitiveComponent* pGltfComponent =
      Cast<UCesiumGltfPrimitiveComponent>(component);
  if (!IsValid(pGltfComponent)) {
    return FCesiumMetadataPrimitive();
  }

  return pGltfComponent->Metadata;
}

TMap<FString, FCesiumMetadataGenericValue>
UCesiumMetadataUtilityBlueprintLibrary::GetMetadataValuesForFace(
    const UPrimitiveComponent* component,
    int64 faceID) {
  const UCesiumGltfPrimitiveComponent* pGltfComponent =
      Cast<UCesiumGltfPrimitiveComponent>(component);
  if (!IsValid(pGltfComponent)) {
    return TMap<FString, FCesiumMetadataGenericValue>();
  }

  const FCesiumMetadataPrimitive& metadata = pGltfComponent->Metadata;
  const TArray<FCesiumVertexMetadata>& vertexFeatures =
      UCesiumMetadataPrimitiveBlueprintLibrary::GetVertexFeatures(metadata);
  if (vertexFeatures.Num() == 0) {
    return TMap<FString, FCesiumMetadataGenericValue>();
  }

  // For now, only considers the first feature
  // TODO: expand to arbitrary number of features once testing data is
  // available
  const FCesiumMetadataFeatureTable& featureTable =
      UCesiumVertexMetadataBlueprintLibrary::GetFeatureTable(vertexFeatures[0]);
  int64 featureID = GetFeatureIDForFace(metadata, vertexFeatures[0], faceID);
  if (featureID < 0) {
    return TMap<FString, FCesiumMetadataGenericValue>();
  }

  return UCesiumMetadataFeatureTableBlueprintLibrary::
      GetMetadataValuesForFeatureID(featureTable, featureID);
}

TMap<FString, FString>
UCesiumMetadataUtilityBlueprintLibrary::GetMetadataValuesAsStringForFace(
    const UPrimitiveComponent* component,
    int64 faceID) {
  const UCesiumGltfPrimitiveComponent* pGltfComponent =
      Cast<UCesiumGltfPrimitiveComponent>(component);
  if (!IsValid(pGltfComponent)) {
    return TMap<FString, FString>();
  }

  const FCesiumMetadataPrimitive& metadata = pGltfComponent->Metadata;
  const TArray<FCesiumVertexMetadata>& vertexFeatures =
      UCesiumMetadataPrimitiveBlueprintLibrary::GetVertexFeatures(metadata);
  if (vertexFeatures.Num() == 0) {
    return TMap<FString, FString>();
  }

  // For now, only considers the first feature
  // TODO: expand to arbitrary number of features once testing data is
  // available
  const FCesiumMetadataFeatureTable& featureTable =
      UCesiumVertexMetadataBlueprintLibrary::GetFeatureTable(vertexFeatures[0]);
  int64 featureID = GetFeatureIDForFace(metadata, vertexFeatures[0], faceID);
  if (featureID < 0) {
    return TMap<FString, FString>();
  }

  return UCesiumMetadataFeatureTableBlueprintLibrary::
      GetMetadataValuesAsStringForFeatureID(featureTable, featureID);
}

int64 UCesiumMetadataUtilityBlueprintLibrary::GetFeatureIDForFace(
    UPARAM(ref) const FCesiumMetadataPrimitive& Primitive,
    UPARAM(ref) const FCesiumVertexMetadata& VertexMetadata,
    int64 faceID) {
  return UCesiumVertexMetadataBlueprintLibrary::GetFeatureIDForVertex(
      VertexMetadata,
      UCesiumMetadataPrimitiveBlueprintLibrary::GetFirstVertexIDFromFaceID(
          Primitive,
          faceID));
}
