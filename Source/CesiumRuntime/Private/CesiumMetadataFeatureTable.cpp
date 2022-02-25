// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumMetadataFeatureTable.h"
#include "CesiumGltf/MetadataFeatureTableView.h"
#include "CesiumMetadataPrimitive.h"

FCesiumMetadataFeatureTable::FCesiumMetadataFeatureTable(
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
UCesiumMetadataFeatureTableBlueprintLibrary::GetMetadataValuesForFeatureID(
    UPARAM(ref) const FCesiumMetadataFeatureTable& FeatureTable,
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

TMap<FString, FString> UCesiumMetadataFeatureTableBlueprintLibrary::
    GetMetadataValuesAsStringForFeatureID(
        UPARAM(ref) const FCesiumMetadataFeatureTable& FeatureTable,
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

int64 UCesiumMetadataFeatureTableBlueprintLibrary::GetNumberOfFeatures(
    UPARAM(ref) const FCesiumMetadataFeatureTable& FeatureTable) {
  if (FeatureTable._properties.Num() == 0) {
    return 0;
  }

  return UCesiumMetadataPropertyBlueprintLibrary::GetNumberOfFeatures(
      FeatureTable._properties.begin().Value());
}

const TMap<FString, FCesiumMetadataProperty>&
UCesiumMetadataFeatureTableBlueprintLibrary::GetProperties(
    UPARAM(ref) const FCesiumMetadataFeatureTable& FeatureTable) {
  return FeatureTable._properties;
}
