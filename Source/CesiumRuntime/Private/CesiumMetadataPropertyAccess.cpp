// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumMetadataPropertyAccess.h"

#include "CesiumFeatureIdSet.h"
#include "CesiumModelMetadata.h"
#include "CesiumPrimitiveFeatures.h"

/*static*/
const FCesiumPropertyTableProperty*
FCesiumMetadataPropertyAccess::FindValidProperty(
    const FCesiumPrimitiveFeatures& Features,
    const FCesiumModelMetadata& Metadata,
    const FString& PropertyName,
    int64 FeatureIDSetIndex) {
  const TArray<FCesiumFeatureIdSet>& featureIDSets =
      UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSets(Features);

  if (FeatureIDSetIndex < 0 || FeatureIDSetIndex >= featureIDSets.Num()) {
    return nullptr;
  }

  const FCesiumFeatureIdSet& featureIDSet =
      featureIDSets[FeatureIDSetIndex];
  const int64 propertyTableIndex =
      UCesiumFeatureIdSetBlueprintLibrary::GetPropertyTableIndex(
          featureIDSet);

  const TArray<FCesiumPropertyTable>& propertyTables =
      UCesiumModelMetadataBlueprintLibrary::GetPropertyTables(Metadata);
  if (propertyTableIndex < 0 || propertyTableIndex >= propertyTables.Num()) {
    return nullptr;
  }
  const FCesiumPropertyTableProperty& propWithName =
      UCesiumPropertyTableBlueprintLibrary::FindProperty(
          propertyTables[propertyTableIndex],
          PropertyName);
  const ECesiumPropertyTablePropertyStatus status =
      UCesiumPropertyTablePropertyBlueprintLibrary::
          GetPropertyTablePropertyStatus(propWithName);
  if (status != ECesiumPropertyTablePropertyStatus::Valid) {
    return nullptr;
  }
  return &propWithName;
}
