// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumMetadataUtilityBlueprintLibrary.h"
#include "CesiumFeatureIdAttribute.h"
#include "CesiumFeatureIdTexture.h"
#include "CesiumGltfComponent.h"
#include "CesiumGltfPrimitiveComponent.h"

static FCesiumModelMetadata EmptyModelMetadata;
static FCesiumPrimitiveMetadata EmptyPrimitiveMetadata;

const FCesiumModelMetadata&
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

const FCesiumPrimitiveMetadata&
UCesiumMetadataUtilityBlueprintLibrary::GetPrimitiveMetadata(
    const UPrimitiveComponent* component) {
  const UCesiumGltfPrimitiveComponent* pGltfComponent =
      Cast<UCesiumGltfPrimitiveComponent>(component);
  if (!IsValid(pGltfComponent)) {
    return EmptyPrimitiveMetadata;
  }

  return pGltfComponent->Metadata;
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

  const UCesiumGltfComponent* pModel =
      Cast<UCesiumGltfComponent>(pGltfComponent->GetOuter());
  if (!IsValid(pModel)) {
    return TMap<FString, FCesiumMetadataValue>();
  }

  const FCesiumPrimitiveFeatures& features = pGltfComponent->Features;
  const TArray<FCesiumFeatureIdSet>& featureIDSets =
      UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSetsOfType(
          features,
          ECesiumFeatureIdSetType::Attribute);
  if (featureIDSets.Num() == 0) {
    return TMap<FString, FCesiumMetadataValue>();
  }

  const FCesiumModelMetadata& modelMetadata = pModel->Metadata;
  const FCesiumPrimitiveMetadata& primitiveMetadata = pGltfComponent->Metadata;

  // For now, only considers the first feature ID set
  // TODO: expand to arbitrary number of features once testing data is
  // available
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
          featureIDSet,
          FaceIndex);
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
