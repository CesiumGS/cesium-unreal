// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumPropertyAttribute.h"
#include <CesiumGltf/PropertyAttributeView.h>
#include "CesiumRuntime.h"

static FCesiumPropertyAttributeProperty EmptyPropertyAttributeProperty;

FCesiumPropertyAttribute::FCesiumPropertyAttribute(
    const CesiumGltf::Model& model,
    const CesiumGltf::MeshPrimitive& primitive,
    const CesiumGltf::PropertyAttribute& propertyAttribute,
    const TSharedPtr<FCesiumMetadataEnumCollection>& pEnumCollection)
    : _status(
          ECesiumPropertyAttributeStatus::ErrorInvalidPropertyAttributeClass),
      _name(propertyAttribute.name.value_or("").c_str()),
      _className(propertyAttribute.classProperty.c_str()),
      _elementCount(0),
      _properties() {
  CesiumGltf::PropertyAttributeView propertyAttributeView{
      model,
      propertyAttribute};
  switch (propertyAttributeView.status()) {
  case CesiumGltf::PropertyAttributeViewStatus::Valid:
    _status = ECesiumPropertyAttributeStatus::Valid;
    break;
  default:
    // Status was already set in initializer list.
    return;
  }

  const CesiumGltf::ExtensionModelExtStructuralMetadata* pExtension =
      model.getExtension<CesiumGltf::ExtensionModelExtStructuralMetadata>();
  // If there was no schema, we would've gotten ErrorMissingSchema for the
  // propertyAttributeView status.
  check(pExtension != nullptr && pExtension->schema != nullptr);

  propertyAttributeView.forEachProperty(
      primitive,
      [&properties = _properties,
       &elementCount = _elementCount,
       &Schema = *pExtension->schema,
       &propertyAttributeView,
       &pEnumCollection](
          const std::string& propertyName,
          auto propertyValue) mutable {
        FString key(UTF8_TO_TCHAR(propertyName.data()));
        const CesiumGltf::ClassProperty* pClassProperty =
            propertyAttributeView.getClassProperty(propertyName);
        check(pClassProperty);

        TSharedPtr<FCesiumMetadataEnum> pEnumDefinition(nullptr);
        if (pEnumCollection.IsValid() && pClassProperty->enumType.has_value()) {
          pEnumDefinition = pEnumCollection->Get(
              FString(UTF8_TO_TCHAR(pClassProperty->enumType.value().c_str())));
        }

        if (elementCount == 0) {
          // Use the first non-zero property size to compare against all the
          // others.
          elementCount = propertyValue.size();
        }

        if (propertyValue.size() > 0 && elementCount != propertyValue.size()) {
          UE_LOG(
              LogCesium,
              Error,
              TEXT(
                  "The size of one or more property attribute properties does not match the others."));
        }

        properties.Add(
            key,
            FCesiumPropertyAttributeProperty(propertyValue, pEnumDefinition));
      });
}

/*static*/ ECesiumPropertyAttributeStatus
UCesiumPropertyAttributeBlueprintLibrary::GetPropertyAttributeStatus(
    UPARAM(ref) const FCesiumPropertyAttribute& PropertyAttribute) {
  return PropertyAttribute._status;
}

/*static*/ const FString&
UCesiumPropertyAttributeBlueprintLibrary::GetPropertyAttributeName(
    UPARAM(ref) const FCesiumPropertyAttribute& PropertyAttribute) {
  return PropertyAttribute._name;
}

/*static*/ const TMap<FString, FCesiumPropertyAttributeProperty>&
UCesiumPropertyAttributeBlueprintLibrary::GetProperties(
    UPARAM(ref) const FCesiumPropertyAttribute& PropertyAttribute) {
  return PropertyAttribute._properties;
}

/*static*/ const TArray<FString>
UCesiumPropertyAttributeBlueprintLibrary::GetPropertyNames(
    UPARAM(ref) const FCesiumPropertyAttribute& PropertyAttribute) {
  TArray<FString> names;
  PropertyAttribute._properties.GenerateKeyArray(names);
  return names;
}

/*static*/ const FCesiumPropertyAttributeProperty&
UCesiumPropertyAttributeBlueprintLibrary::FindProperty(
    UPARAM(ref) const FCesiumPropertyAttribute& PropertyAttribute,
    const FString& PropertyName) {
  const FCesiumPropertyAttributeProperty* property =
      PropertyAttribute._properties.Find(PropertyName);
  return property ? *property : EmptyPropertyAttributeProperty;
}

/*static*/ TMap<FString, FCesiumMetadataValue>
UCesiumPropertyAttributeBlueprintLibrary::GetMetadataValuesAtIndex(
    UPARAM(ref) const FCesiumPropertyAttribute& PropertyAttribute,
    int64 Index) {
  TMap<FString, FCesiumMetadataValue> values;
  if (Index < 0 || Index >= PropertyAttribute._elementCount) {
    return values;
  }

  for (const auto& pair : PropertyAttribute._properties) {
    const FCesiumPropertyAttributeProperty& property = pair.Value;
    ECesiumPropertyAttributePropertyStatus status =
        UCesiumPropertyAttributePropertyBlueprintLibrary::
            GetPropertyAttributePropertyStatus(property);
    if (status == ECesiumPropertyAttributePropertyStatus::Valid) {
      values.Add(
          pair.Key,
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetValue(
              pair.Value,
              Index));
    } else if (
        status ==
        ECesiumPropertyAttributePropertyStatus::EmptyPropertyWithDefault) {
      values.Add(
          pair.Key,
          UCesiumPropertyAttributePropertyBlueprintLibrary::GetDefaultValue(
              pair.Value));
    }
  }

  return values;
}
