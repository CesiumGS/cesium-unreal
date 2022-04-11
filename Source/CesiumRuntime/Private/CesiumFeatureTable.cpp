// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumFeatureTable.h"
#include "CesiumGltf/MetadataFeatureTableView.h"
#include "CesiumMetadataPrimitive.h"

FCesiumFeatureTable::FCesiumFeatureTable(
    const CesiumGltf::Model& model,
    const CesiumGltf::FeatureTable& featureTable) {

  CesiumGltf::MetadataFeatureTableView featureTableView{&model, &featureTable};

  featureTableView.forEachProperty([&properties = _properties](
                                       const std::string& propertyName,
                                       auto propertyValue) mutable {
    if (propertyValue.status() ==
        CesiumGltf::MetadataPropertyViewStatus::Valid) {
      FString key(UTF8_TO_TCHAR(propertyName.data()));
      properties.Add(key, FCesiumMetadataProperty(propertyValue));
    }
  });
}

TMap<FString, FCesiumMetadataGenericValue>
UCesiumFeatureTableBlueprintLibrary::GetMetadataValuesForFeatureID(
    UPARAM(ref) const FCesiumFeatureTable& FeatureTable,
    int64 featureID) {
  TMap<FString, FCesiumMetadataGenericValue> feature;
  for (const auto& pair : FeatureTable._properties) {
    feature.Add(
        pair.Key,
        UCesiumMetadataPropertyBlueprintLibrary::GetGenericValue(
            pair.Value,
            featureID));
  }

  return feature;
}

TMap<FString, FString>
UCesiumFeatureTableBlueprintLibrary::GetMetadataValuesAsStringForFeatureID(
    UPARAM(ref) const FCesiumFeatureTable& FeatureTable,
    int64 featureID) {
  TMap<FString, FString> feature;
  for (const auto& pair : FeatureTable._properties) {
    feature.Add(
        pair.Key,
        UCesiumMetadataGenericValueBlueprintLibrary::GetString(
            UCesiumMetadataPropertyBlueprintLibrary::GetGenericValue(
                pair.Value,
                featureID),
            ""));
  }

  return feature;
}

int64 UCesiumFeatureTableBlueprintLibrary::GetNumberOfFeatures(
    UPARAM(ref) const FCesiumFeatureTable& FeatureTable) {
  if (FeatureTable._properties.Num() == 0) {
    return 0;
  }

  return UCesiumMetadataPropertyBlueprintLibrary::GetNumberOfFeatures(
      FeatureTable._properties.begin().Value());
}

const TMap<FString, FCesiumMetadataProperty>&
UCesiumFeatureTableBlueprintLibrary::GetProperties(
    UPARAM(ref) const FCesiumFeatureTable& FeatureTable) {
  return FeatureTable._properties;
}
