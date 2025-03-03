// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumPropertyTable.h"
#include "CesiumGltf/PropertyTableView.h"

static FCesiumPropertyTableProperty EmptyPropertyTableProperty;

FCesiumPropertyTable::FCesiumPropertyTable(
    const CesiumGltf::Model& Model,
    const CesiumGltf::PropertyTable& PropertyTable,
    const TSharedPtr<FCesiumMetadataEnumCollection>& EnumCollection)
    : _status(ECesiumPropertyTableStatus::ErrorInvalidPropertyTableClass),
      _name(PropertyTable.name.value_or("").c_str()),
      _className(PropertyTable.classProperty.c_str()),
      _count(PropertyTable.count),
      _properties() {
  CesiumGltf::PropertyTableView propertyTableView{Model, PropertyTable};
  switch (propertyTableView.status()) {
  case CesiumGltf::PropertyTableViewStatus::Valid:
    _status = ECesiumPropertyTableStatus::Valid;
    break;
  default:
    // Status was already set in initializer list.
    return;
  }

  const CesiumGltf::ExtensionModelExtStructuralMetadata* pExtension =
      Model.getExtension<CesiumGltf::ExtensionModelExtStructuralMetadata>();
  // If there was no schema, we would've gotten ErrorMissingSchema for the
  // propertyTableView status.
  check(pExtension != nullptr && pExtension->schema != nullptr);

  propertyTableView.forEachProperty([&properties = _properties,
                                     &Schema = *pExtension->schema,
                                     &propertyTableView,
                                     &EnumCollection](
                                        const std::string& propertyName,
                                        auto propertyValue) mutable {
    FString key(UTF8_TO_TCHAR(propertyName.data()));
    const CesiumGltf::ClassProperty* pClassProperty =
        propertyTableView.getClassProperty(propertyName);
    check(pClassProperty);

    TSharedPtr<FCesiumMetadataEnum> EnumDefinition(nullptr);
    if (EnumCollection.IsValid() && pClassProperty->enumType.has_value()) {
      EnumDefinition = EnumCollection->Get(
          FString(UTF8_TO_TCHAR(pClassProperty->enumType.value().c_str())));
    }

    properties.Add(
        key,
        FCesiumPropertyTableProperty(propertyValue, EnumDefinition));
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

/*static*/ int64 UCesiumPropertyTableBlueprintLibrary::GetPropertyTableCount(
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
UCesiumPropertyTableBlueprintLibrary::GetPropertyNames(
    UPARAM(ref) const FCesiumPropertyTable& PropertyTable) {
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
UCesiumPropertyTableBlueprintLibrary::GetMetadataValuesForFeature(
    UPARAM(ref) const FCesiumPropertyTable& PropertyTable,
    int64 FeatureID) {
  TMap<FString, FCesiumMetadataValue> values;
  if (FeatureID < 0 || FeatureID >= PropertyTable._count) {
    return values;
  }

  for (const auto& pair : PropertyTable._properties) {
    const FCesiumPropertyTableProperty& property = pair.Value;
    ECesiumPropertyTablePropertyStatus status =
        UCesiumPropertyTablePropertyBlueprintLibrary::
            GetPropertyTablePropertyStatus(property);
    if (status == ECesiumPropertyTablePropertyStatus::Valid) {
      values.Add(
          pair.Key,
          UCesiumPropertyTablePropertyBlueprintLibrary::GetValue(
              pair.Value,
              FeatureID));
    } else if (
        status ==
        ECesiumPropertyTablePropertyStatus::EmptyPropertyWithDefault) {
      values.Add(
          pair.Key,
          UCesiumPropertyTablePropertyBlueprintLibrary::GetDefaultValue(
              pair.Value));
    }
  }

  return values;
}

/*static*/ TMap<FString, FString>
UCesiumPropertyTableBlueprintLibrary::GetMetadataValuesForFeatureAsStrings(
    UPARAM(ref) const FCesiumPropertyTable& PropertyTable,
    int64 FeatureID) {
  TMap<FString, FString> values;
  if (FeatureID < 0 || FeatureID >= PropertyTable._count) {
    return values;
  }

  for (const auto& pair : PropertyTable._properties) {
    const FCesiumPropertyTableProperty& property = pair.Value;
    ECesiumPropertyTablePropertyStatus status =
        UCesiumPropertyTablePropertyBlueprintLibrary::
            GetPropertyTablePropertyStatus(property);
    if (status == ECesiumPropertyTablePropertyStatus::Valid) {
      values.Add(
          pair.Key,
          UCesiumPropertyTablePropertyBlueprintLibrary::GetString(
              pair.Value,
              FeatureID));
    }
  }

  return values;
}
