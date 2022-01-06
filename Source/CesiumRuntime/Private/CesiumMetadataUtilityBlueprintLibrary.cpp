#include "CesiumMetadataUtilityBlueprintLibrary.h"
#include "CesiumFeatureIDTexture.h"
#include "CesiumGltfComponent.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumVertexMetadata.h"

static FCesiumMetadataModel EmptyModelMetadata;
static FCesiumMetadataPrimitive EmptyPrimitiveMetadata;

const FCesiumMetadataModel&
UCesiumMetadataUtilityBlueprintLibrary::GetModelMetadata(
    const UPrimitiveComponent* component) {
  const UCesiumGltfPrimitiveComponent* pGltfComponent =
      Cast<UCesiumGltfPrimitiveComponent>(component);

  if (!IsValid(pGltfComponent)) {
    return EmptyModelMetadata;
  }

  const UCesiumGltfComponent* pModel =
      Cast<UCesiumGltfComponent>(pGltfComponent->GetOuter());
  if (!IsValid(pModel)) {
    return EmptyModelMetadata;
  }

  return pModel->Metadata;
}

const FCesiumMetadataPrimitive&
UCesiumMetadataUtilityBlueprintLibrary::GetPrimitiveMetadata(
    const UPrimitiveComponent* component) {
  const UCesiumGltfPrimitiveComponent* pGltfComponent =
      Cast<UCesiumGltfPrimitiveComponent>(component);
  if (!IsValid(pGltfComponent)) {
    return EmptyPrimitiveMetadata;
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

  const UCesiumGltfComponent* pModel =
      Cast<UCesiumGltfComponent>(pGltfComponent->GetOuter());
  if (!IsValid(pModel)) {
    return TMap<FString, FCesiumMetadataGenericValue>();
  }

  const FCesiumMetadataModel& modelMetadata = pModel->Metadata;
  const FCesiumMetadataPrimitive& primitiveMetadata = pGltfComponent->Metadata;
  const TArray<FCesiumVertexMetadata>& vertexFeatures =
      UCesiumMetadataPrimitiveBlueprintLibrary::GetVertexFeatures(
          primitiveMetadata);
  if (vertexFeatures.Num() == 0) {
    return TMap<FString, FCesiumMetadataGenericValue>();
  }

  // For now, only considers the first feature
  // TODO: expand to arbitrary number of features once testing data is
  // available
  const TMap<FString, FCesiumMetadataFeatureTable>& featureTables =
      UCesiumMetadataModelBlueprintLibrary::GetFeatureTables(modelMetadata);
  const FString& featureTableName =
      UCesiumVertexMetadataBlueprintLibrary::GetFeatureTableName(
          vertexFeatures[0]);
  const FCesiumMetadataFeatureTable* pFeatureTable =
      featureTables.Find(featureTableName);
  if (!pFeatureTable) {
    return TMap<FString, FCesiumMetadataGenericValue>();
  }

  int64 featureID =
      GetFeatureIDForFace(primitiveMetadata, vertexFeatures[0], faceID);
  if (featureID < 0) {
    return TMap<FString, FCesiumMetadataGenericValue>();
  }

  return UCesiumMetadataFeatureTableBlueprintLibrary::
      GetMetadataValuesForFeatureID(*pFeatureTable, featureID);
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

  const UCesiumGltfComponent* pModel =
      Cast<UCesiumGltfComponent>(pGltfComponent->GetOuter());
  if (!IsValid(pModel)) {
    return TMap<FString, FString>();
  }

  const FCesiumMetadataModel& modelMetadata = pModel->Metadata;
  const FCesiumMetadataPrimitive& primitiveMetadata = pGltfComponent->Metadata;
  const TArray<FCesiumVertexMetadata>& vertexFeatures =
      UCesiumMetadataPrimitiveBlueprintLibrary::GetVertexFeatures(
          primitiveMetadata);
  if (vertexFeatures.Num() == 0) {
    return TMap<FString, FString>();
  }

  // For now, only considers the first feature
  // TODO: expand to arbitrary number of features once testing data is
  // available
  const TMap<FString, FCesiumMetadataFeatureTable>& featureTables =
      UCesiumMetadataModelBlueprintLibrary::GetFeatureTables(modelMetadata);
  const FString& featureTableName =
      UCesiumVertexMetadataBlueprintLibrary::GetFeatureTableName(
          vertexFeatures[0]);
  const FCesiumMetadataFeatureTable* pFeatureTable =
      featureTables.Find(featureTableName);
  if (!pFeatureTable) {
    return TMap<FString, FString>();
  }

  int64 featureID =
      GetFeatureIDForFace(primitiveMetadata, vertexFeatures[0], faceID);
  if (featureID < 0) {
    return TMap<FString, FString>();
  }

  return UCesiumMetadataFeatureTableBlueprintLibrary::
      GetMetadataValuesAsStringForFeatureID(*pFeatureTable, featureID);
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
