// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumMetadataUtilityBlueprintLibrary.h"
#include "CesiumFeatureIdAttribute.h"
#include "CesiumFeatureIdTexture.h"
#include "CesiumGltfComponent.h"
#include "CesiumGltfPrimitiveComponent.h"

// REMOVE AFTER DEPRECATION
#include "CesiumMetadataFeatureTable.h"

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
  const TArray<FCesiumFeatureIDAttribute>& featureIdAttributes =
      UCesiumMetadataPrimitiveBlueprintLibrary::GetFeatureIdAttributes(
          primitiveMetadata);
  if (featureIdAttributes.Num() == 0) {
    return TMap<FString, FCesiumMetadataGenericValue>();
  }

  // For now, only considers the first feature
  // TODO: expand to arbitrary number of features once testing data is
  // available
  const TMap<FString, FCesiumFeatureTable>& featureTables =
      UCesiumMetadataModelBlueprintLibrary::GetFeatureTables(modelMetadata);
  const FString& featureTableName = "";
  const FCesiumFeatureTable* pFeatureTable =
      featureTables.Find(featureTableName);
  if (!pFeatureTable) {
    return TMap<FString, FCesiumMetadataGenericValue>();
  }

  int64 featureID = -1;
      //GetFeatureIDFromFaceID(primitiveMetadata, featureIdAttributes[0], faceID);
  if (featureID < 0) {
    return TMap<FString, FCesiumMetadataGenericValue>();
  }

  return UCesiumFeatureTableBlueprintLibrary::GetMetadataValuesForFeatureID(
      *pFeatureTable,
      featureID);
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
  const TArray<FCesiumFeatureIDAttribute>& featureIdAttributes =
      UCesiumMetadataPrimitiveBlueprintLibrary::GetFeatureIdAttributes(
          primitiveMetadata);
  if (featureIdAttributes.Num() == 0) {
    return TMap<FString, FString>();
  }

  // For now, only considers the first feature
  // TODO: expand to arbitrary number of features once testing data is
  // available
  const TMap<FString, FCesiumFeatureTable>& featureTables =
      UCesiumMetadataModelBlueprintLibrary::GetFeatureTables(modelMetadata);
  const FString& featureTableName = "";
  const FCesiumFeatureTable* pFeatureTable =
      featureTables.Find(featureTableName);
  if (!pFeatureTable) {
    return TMap<FString, FString>();
  }

  int64 featureID = -1; // GetFeatureIDFromFaceID(primitiveMetadata,
                        // featureIdAttributes[0], faceID);
  if (featureID < 0) {
    return TMap<FString, FString>();
  }

  return UCesiumFeatureTableBlueprintLibrary::
      GetMetadataValuesAsStringForFeatureID(*pFeatureTable, featureID);
}
