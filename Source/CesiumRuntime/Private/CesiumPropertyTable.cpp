// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumPropertyTable.h"
#include "CesiumGltf/StructuralMetadataPropertyTableView.h"
#include "CesiumMetadataPrimitive.h"

using namespace CesiumGltf;
using namespace CesiumGltf::StructuralMetadata;

FCesiumPropertyTable::FCesiumPropertyTable(
    const Model& Model,
    const ExtensionExtStructuralMetadataPropertyTable& PropertyTable)
    : _status(ECesiumPropertyTableStatus::ErrorInvalidMetadataExtension),
      _count(PropertyTable.count),
      _properties() {

  PropertyTableView propertyTableView{Model, PropertyTable};
  switch (propertyTableView.status()) {
  case PropertyTableViewStatus::Valid:
    break;
  case PropertyTableViewStatus::ErrorClassNotFound:
    _status = ECesiumPropertyTableStatus::ErrorInvalidPropertyTableClass;
    return;
  default:
    // Status was already set in initializer list.
    return;
  }

  propertyTableView.forEachProperty([&properties = _properties](
                                        const std::string& propertyName,
                                        auto propertyValue) mutable {
    // Should we log a warning if invalid? I think it's actually nicer to
    // include the property but say it's invalid, so the user isn't confused.
    if (propertyValue.status() == PropertyTablePropertyViewStatus::Valid) {
      FString key(UTF8_TO_TCHAR(propertyName.data()));
      properties.Add(key, FCesiumPropertyTableProperty(propertyValue));
    }
  });
}

ECesiumPropertyTableStatus
UCesiumPropertyTableBlueprintLibrary::GetPropertyTableStatus(
    UPARAM(ref) const FCesiumPropertyTable& PropertyTable) {
  return PropertyTable._status;
}

int64 UCesiumPropertyTableBlueprintLibrary::GetNumberOfElements(
    UPARAM(ref) const FCesiumPropertyTable& PropertyTable) {
  if (PropertyTable._status != ECesiumPropertyTableStatus::Valid) {
    return 0;
  }

  return PropertyTable._count;
}

const TMap<FString, FCesiumPropertyTableProperty>&
UCesiumPropertyTableBlueprintLibrary::GetProperties(
    UPARAM(ref) const FCesiumPropertyTable& PropertyTable) {
  return PropertyTable._properties;
}

TMap<FString, FCesiumMetadataValue>
UCesiumPropertyTableBlueprintLibrary::GetMetadataValuesForFeatureID(
    UPARAM(ref) const FCesiumPropertyTable& PropertyTable,
    int64 featureID) {
  TMap<FString, FCesiumMetadataValue> feature;
  for (const auto& pair : PropertyTable._properties) {
    feature.Add(
        pair.Key,
        UCesiumPropertyTablePropertyBlueprintLibrary::GetGenericValue(
            pair.Value,
            featureID));
  }

  return feature;
}

TMap<FString, FString>
UCesiumPropertyTableBlueprintLibrary::GetMetadataValuesAsStringsForFeatureID(
    UPARAM(ref) const FCesiumPropertyTable& PropertyTable,
    int64 featureID) {
  TMap<FString, FString> feature;
  for (const auto& pair : PropertyTable._properties) {
    feature.Add(
        pair.Key,
        UCesiumMetadataValueBlueprintLibrary::GetString(
            UCesiumPropertyTablePropertyBlueprintLibrary::GetGenericValue(
                pair.Value,
                featureID),
            ""));
  }

  return feature;
}
