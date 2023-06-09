// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumPropertyTexture.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltf/PropertyTexturePropertyView.h"
#include "CesiumGltf/PropertyTextureView.h"

using namespace CesiumGltf;

FCesiumPropertyTexture::FCesiumPropertyTexture(
    const CesiumGltf::Model& InModel,
    const CesiumGltf::ExtensionExtStructuralMetadataPropertyTexture&
        PropertyTexture)
    : _status(ECesiumPropertyTextureStatus::ErrorInvalidMetadataExtension),
      _propertyTextureView(InModel, PropertyTexture) {
  switch (this->_propertyTextureView.status()) {
  case PropertyTextureViewStatus::Valid:
    break;
    // TODO: isolate the individual property status from the whole status.
  }

  const std::unordered_map<
      std::string,
      PropertyTexturePropertyView>& properties =
      this->_propertyTextureView.getProperties();
  this->_propertyKeys.Reserve(properties.size());
  for (auto propertyView : properties) {
    this->_propertyKeys.Emplace(UTF8_TO_TCHAR(propertyView.first.c_str()));
  }
}

const TArray<FString>& UCesiumPropertyTextureBlueprintLibrary::GetPropertyNames(
    UPARAM(ref) const FCesiumPropertyTexture& PropertyTexture) {
  return PropertyTexture._propertyKeys;
}

FCesiumPropertyTextureProperty
UCesiumPropertyTextureBlueprintLibrary::FindProperty(
    UPARAM(ref) const FCesiumPropertyTexture& PropertyTexture,
    const FString& PropertyName) {

  const std::unordered_map<
      std::string,
      CesiumGltf::PropertyTexturePropertyView>& properties =
      PropertyTexture._PropertyTextureView.getProperties();

  auto propertyIt = properties.find(TCHAR_TO_UTF8(*PropertyName));
  if (propertyIt == properties.end()) {
    return FCesiumPropertyTextureProperty();
  }

  return FCesiumPropertyTextureProperty(propertyIt->second);
}
