// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumPropertyTexture.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltf/PropertyTexturePropertyView.h"
#include "CesiumGltf/PropertyTextureView.h"

using namespace CesiumGltf;

static FCesiumPropertyTextureProperty EmptyPropertyTextureProperty;

FCesiumPropertyTexture::FCesiumPropertyTexture(
    const CesiumGltf::Model& Model,
    const CesiumGltf::PropertyTexture& PropertyTexture)
    : _status(ECesiumPropertyTextureStatus::ErrorInvalidMetadataExtension),
      _name(PropertyTexture.name.value_or("").c_str()),
      _propertyTextureView(Model, PropertyTexture) {
  switch (this->_propertyTextureView.status()) {
  case PropertyTextureViewStatus::Valid:
    break;
  case PropertyTextureViewStatus::ErrorMissingMetadataExtension:
  default:
    return;
    // TODO: isolate the individual property status from the whole status.
  }

  //propertyTableView.forEachProperty([&properties = _properties](
  //                                      const std::string& propertyName,
  //                                      auto propertyValue) mutable {
  //  FString key(UTF8_TO_TCHAR(propertyName.data()));
  //  if (propertyValue.status() == PropertyTablePropertyViewStatus::Valid) {
  //    properties.Add(key, FCesiumPropertyTableProperty(propertyValue));
  //  } else {
  //    properties.Add(key, EmptyPropertyTableProperty);
  //  }
  //});
  const std::unordered_map<std::string, PropertyTexturePropertyView>&
      properties = this->_propertyTextureView.getProperties();
  this->_properties.Reserve(properties.size());
  for (auto propertyView : properties) {
    this->_properties.Emplace(
        FString(propertyView.first.c_str()),
        FCesiumPropertyTextureProperty(propertyView.second));
  }
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

/*static*/ const TArray<FCesiumPropertyTextureProperty>
UCesiumPropertyTextureBlueprintLibrary::GetProperties(
    UPARAM(ref) const FCesiumPropertyTexture& PropertyTexture) {
  TArray<FCesiumPropertyTextureProperty> properties;
  PropertyTexture._properties.GenerateValueArray(properties);
  return properties;
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
