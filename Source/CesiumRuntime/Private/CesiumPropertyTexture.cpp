// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumPropertyTexture.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltf/PropertyTexturePropertyView.h"
#include "CesiumGltf/PropertyTextureView.h"

using namespace CesiumGltf;

static FCesiumPropertyTextureProperty EmptyPropertyTextureProperty;

FCesiumPropertyTexture::FCesiumPropertyTexture(
    const CesiumGltf::Model& Model,
    const CesiumGltf::PropertyTexture& PropertyTexture)
    : _status(ECesiumPropertyTextureStatus::ErrorInvalidPropertyTextureClass),
      _name(PropertyTexture.name.value_or("").c_str()),
      _className(PropertyTexture.classProperty.c_str()) {
  PropertyTextureView propertyTextureView(Model, PropertyTexture);
  switch (propertyTextureView.status()) {
  case PropertyTextureViewStatus::Valid:
    _status = ECesiumPropertyTextureStatus::Valid;
    break;
  default:
    // Status was already set in initializer list.
    return;
  }

  propertyTextureView.forEachProperty([&properties = _properties](
                                          const std::string& propertyName,
                                          auto propertyValue) mutable {
    FString key(UTF8_TO_TCHAR(propertyName.data()));
    properties.Add(key, FCesiumPropertyTextureProperty(propertyValue));
  });
}

/*static*/ const ECesiumPropertyTextureStatus
UCesiumPropertyTextureBlueprintLibrary::GetPropertyTextureStatus(
    UPARAM(ref) const FCesiumPropertyTexture& PropertyTexture) {
  return PropertyTexture._status;
}

/*static*/ const FString&
UCesiumPropertyTextureBlueprintLibrary::GetPropertyTextureName(
    UPARAM(ref) const FCesiumPropertyTexture& PropertyTexture) {
  return PropertyTexture._name;
}

/*static*/ const TMap<FString, FCesiumPropertyTextureProperty>
UCesiumPropertyTextureBlueprintLibrary::GetProperties(
    UPARAM(ref) const FCesiumPropertyTexture& PropertyTexture) {
  return PropertyTexture._properties;
}

/*static*/ const TArray<FString>
UCesiumPropertyTextureBlueprintLibrary::GetPropertyNames(
    UPARAM(ref) const FCesiumPropertyTexture& PropertyTexture) {
  TArray<FString> names;
  PropertyTexture._properties.GenerateKeyArray(names);
  return names;
}

const FCesiumPropertyTextureProperty&
UCesiumPropertyTextureBlueprintLibrary::FindProperty(
    UPARAM(ref) const FCesiumPropertyTexture& PropertyTexture,
    const FString& PropertyName) {
  const FCesiumPropertyTextureProperty* property =
      PropertyTexture._properties.Find(PropertyName);
  return property ? *property : EmptyPropertyTextureProperty;
}

TMap<FString, FCesiumMetadataValue>
UCesiumPropertyTextureBlueprintLibrary::GetMetadataValuesForUV(
    UPARAM(ref) const FCesiumPropertyTexture& PropertyTexture,
    const FVector2D& UV) {
  TMap<FString, FCesiumMetadataValue> values;

  for (const auto& pair : PropertyTexture._properties) {
    const FCesiumPropertyTextureProperty& property = pair.Value;
    ECesiumPropertyTexturePropertyStatus status =
        UCesiumPropertyTexturePropertyBlueprintLibrary::
            GetPropertyTexturePropertyStatus(property);
    if (status == ECesiumPropertyTexturePropertyStatus::Valid) {
      values.Add(
          pair.Key,
          UCesiumPropertyTexturePropertyBlueprintLibrary::GetValue(
              pair.Value,
              UV));
    } else if (
        status ==
        ECesiumPropertyTexturePropertyStatus::EmptyPropertyWithDefault) {
      values.Add(
          pair.Key,
          UCesiumPropertyTexturePropertyBlueprintLibrary::GetDefaultValue(
              pair.Value));
    }
  }

  return values;
}

TMap<FString, FString>
UCesiumPropertyTextureBlueprintLibrary::GetMetadataValuesForUVAsStrings(
    UPARAM(ref) const FCesiumPropertyTexture& PropertyTexture,
    const FVector2D& UV) {
  TMap<FString, FString> values;

  for (const auto& pair : PropertyTexture._properties) {
    const FCesiumPropertyTextureProperty& property = pair.Value;
    ECesiumPropertyTexturePropertyStatus status =
        UCesiumPropertyTexturePropertyBlueprintLibrary::
            GetPropertyTexturePropertyStatus(property);
    if (status == ECesiumPropertyTexturePropertyStatus::Valid) {
      FCesiumMetadataValue value =
          UCesiumPropertyTexturePropertyBlueprintLibrary::GetValue(
              pair.Value,
              UV);
      values.Add(
          pair.Key,
          UCesiumMetadataValueBlueprintLibrary::GetString(value, FString()));
    } else if (
        status ==
        ECesiumPropertyTexturePropertyStatus::EmptyPropertyWithDefault) {
      FCesiumMetadataValue value =
          UCesiumPropertyTexturePropertyBlueprintLibrary::GetDefaultValue(
              pair.Value);
      values.Add(
          pair.Key,
          UCesiumMetadataValueBlueprintLibrary::GetString(value, FString()));
    }
  }

  return values;
}
