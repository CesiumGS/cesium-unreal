// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumPropertyTable.h"
#include "CesiumGltf/StructuralMetadataPropertyTableView.h"
#include "CesiumMetadataPrimitive.h"

using namespace CesiumGltf;
using namespace CesiumGltf::StructuralMetadata;

static FCesiumPropertyTableProperty EmptyPropertyTableProperty;

FCesiumPropertyTable::FCesiumPropertyTable(
    const Model& Model,
    const PropertyTable& PropertyTable)
    : _status(ECesiumPropertyTableStatus::ErrorInvalidMetadataExtension),
      _count(PropertyTable.count),
      _name(),
      _properties() {
  _name = PropertyTable.name.value_or("").c_str();

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
    FString key(UTF8_TO_TCHAR(propertyName.data()));
    if (propertyValue.status() == PropertyTablePropertyViewStatus::Valid) {
      properties.Add(key, FCesiumPropertyTableProperty(propertyValue));
    } else {
      properties.Add(key, EmptyPropertyTableProperty);
    }
  });
}

/*static*/ ECesiumPropertyTableStatus
UCesiumPropertyTableBlueprintLibrary::GetPropertyTableStatus(
    UPARAM(ref) const FCesiumPropertyTable& PropertyTable) {
  return PropertyTable._status;
}

/*static*/ const FString&
UCesiumPropertyTableBlueprintLibrary::GetPropertyTableName(
    UPARAM(ref) const FCesiumPropertyTable& PropertyTable) {
  return PropertyTable._name;
}

/*static*/ int64 UCesiumPropertyTableBlueprintLibrary::GetPropertyTableSize(
    UPARAM(ref) const FCesiumPropertyTable& PropertyTable) {
  if (PropertyTable._status != ECesiumPropertyTableStatus::Valid) {
    return 0;
  }

  return PropertyTable._count;
}

/*static*/ const TMap<FString, FCesiumPropertyTableProperty>&
UCesiumPropertyTableBlueprintLibrary::GetProperties(
    UPARAM(ref) const FCesiumPropertyTable& PropertyTable) {
  return PropertyTable._properties;
}

/*static*/ const TArray<FString>
UCesiumPropertyTableBlueprintLibrary::GetPropertyNames(UPARAM(ref) const FCesiumPropertyTable& PropertyTable) {
  TArray<FString> names;
  PropertyTable._properties.GenerateKeyArray(names);
  return names;
}

/*static*/ const FCesiumPropertyTableProperty&
UCesiumPropertyTableBlueprintLibrary::FindProperty(
  UPARAM(ref) const FCesiumPropertyTable& PropertyTable,
    const FString& PropertyName) {
  const FCesiumPropertyTableProperty* property =
      PropertyTable._properties.Find(PropertyName);
  return property ? *property : EmptyPropertyTableProperty;
}

/*static*/ TMap<FString, FCesiumMetadataValue>
UCesiumPropertyTableBlueprintLibrary::GetMetadataValuesForFeatureID(
    UPARAM(ref) const FCesiumPropertyTable& PropertyTable,
    int64 featureID) {
  TMap<FString, FCesiumMetadataValue> values;
  for (const auto& pair : PropertyTable._properties) {
    values.Add(
        pair.Key,
        UCesiumPropertyTablePropertyBlueprintLibrary::GetGenericValue(
            pair.Value,
            featureID));
  }

  return values;
}

/*static*/ TMap<FString, FString>
UCesiumPropertyTableBlueprintLibrary::GetMetadataValuesAsStringsForFeatureID(
    UPARAM(ref) const FCesiumPropertyTable& PropertyTable,
    int64 featureID) {
  TMap<FString, FString> values;
  for (const auto& pair : PropertyTable._properties) {
    values.Add(
        pair.Key,
        UCesiumMetadataValueBlueprintLibrary::GetString(
            UCesiumPropertyTablePropertyBlueprintLibrary::GetGenericValue(
                pair.Value,
                featureID),
            ""));
  }

  return values;
}
