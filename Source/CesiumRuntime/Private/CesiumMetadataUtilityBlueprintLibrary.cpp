// Copyright 2020-2024 CesiumGS, Inc. and Contributors

PRAGMA_DISABLE_DEPRECATION_WARNINGS

#include "CesiumMetadataUtilityBlueprintLibrary.h"
#include "CesiumFeatureIdTexture.h"
#include "CesiumGltfComponent.h"
#include "CesiumGltfPrimitiveComponent.h"

static FCesiumMetadataPrimitive EmptyMetadataPrimitive;

const FCesiumMetadataPrimitive&
UCesiumMetadataUtilityBlueprintLibrary::GetPrimitiveMetadata(
    const UPrimitiveComponent* component) {
  const UCesiumGltfPrimitiveComponent* pGltfComponent =
      Cast<UCesiumGltfPrimitiveComponent>(component);
  if (!IsValid(pGltfComponent)) {
    return EmptyMetadataPrimitive;
  }

  return pGltfComponent->getPrimitiveData().Metadata_DEPRECATED;
}

TMap<FString, FCesiumMetadataValue>
UCesiumMetadataUtilityBlueprintLibrary::GetMetadataValuesForFace(
    const UPrimitiveComponent* component,
    int64 FaceIndex) {
  const UCesiumGltfPrimitiveComponent* pGltfComponent =
      Cast<UCesiumGltfPrimitiveComponent>(component);
  if (!IsValid(pGltfComponent)) {
    return TMap<FString, FCesiumMetadataValue>();
  }

  const CesiumPrimitiveData& primData = pGltfComponent->getPrimitiveData();
  const UCesiumGltfComponent* pModel =
      Cast<UCesiumGltfComponent>(pGltfComponent->GetOuter());
  if (!IsValid(pModel)) {
    return TMap<FString, FCesiumMetadataValue>();
  }

  const FCesiumPrimitiveFeatures& features = primData.Features;
  const TArray<FCesiumFeatureIdSet>& featureIDSets =
      UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSetsOfType(
          features,
          ECesiumFeatureIdSetType::Attribute);
  if (featureIDSets.Num() == 0) {
    return TMap<FString, FCesiumMetadataValue>();
  }

  const FCesiumModelMetadata& modelMetadata = pModel->Metadata;
  const FCesiumPrimitiveMetadata& primitiveMetadata = primData.Metadata;

  // For now, only considers the first feature ID set
  const FCesiumFeatureIdSet& featureIDSet = featureIDSets[0];
  const int64 propertyTableIndex =
      UCesiumFeatureIdSetBlueprintLibrary::GetPropertyTableIndex(featureIDSet);

  const TArray<FCesiumPropertyTable>& propertyTables =
      UCesiumModelMetadataBlueprintLibrary::GetPropertyTables(modelMetadata);
  if (propertyTableIndex < 0 || propertyTableIndex >= propertyTables.Num()) {
    return TMap<FString, FCesiumMetadataValue>();
  }

  const FCesiumPropertyTable& propertyTable =
      propertyTables[propertyTableIndex];

  int64 featureID =
      UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDFromFace(
          features,
          FaceIndex,
          0);
  if (featureID < 0) {
    return TMap<FString, FCesiumMetadataValue>();
  }

  return UCesiumPropertyTableBlueprintLibrary::GetMetadataValuesForFeature(
      propertyTable,
      featureID);
}

TMap<FString, FString>
UCesiumMetadataUtilityBlueprintLibrary::GetMetadataValuesAsStringForFace(
    const UPrimitiveComponent* Component,
    int64 FaceIndex) {
  TMap<FString, FCesiumMetadataValue> values =
      UCesiumMetadataUtilityBlueprintLibrary::GetMetadataValuesForFace(
          Component,
          FaceIndex);
  TMap<FString, FString> strings;
  for (auto valuesIt : values) {
    strings.Add(
        valuesIt.Key,
        UCesiumMetadataValueBlueprintLibrary::GetString(valuesIt.Value, ""));
  }

  return strings;
}

int64 UCesiumMetadataUtilityBlueprintLibrary::GetFeatureIDFromFaceID(
    UPARAM(ref) const FCesiumMetadataPrimitive& Primitive,
    UPARAM(ref) const FCesiumFeatureIdAttribute& FeatureIDAttribute,
    int64 FaceID) {
  return UCesiumFeatureIdAttributeBlueprintLibrary::GetFeatureIDForVertex(
      FeatureIDAttribute,
      UCesiumMetadataPrimitiveBlueprintLibrary::GetFirstVertexIDFromFaceID(
          Primitive,
          FaceID));
}

PRAGMA_ENABLE_DEPRECATION_WARNINGS
