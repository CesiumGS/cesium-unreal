// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumFeatureTexture.h"
#include "CesiumGltf/FeatureTexturePropertyView.h"
#include "CesiumGltf/FeatureTextureView.h"
#include "CesiumGltf/Model.h"

FCesiumFeatureTexture::FCesiumFeatureTexture(
    const CesiumGltf::Model& model,
    const CesiumGltf::FeatureTexture& featureTexture)
    : _featureTextureView(model, featureTexture) {

  if (this->_featureTextureView.status() ==
      CesiumGltf::FeatureTextureViewStatus::Valid) {
    const std::unordered_map<
        std::string,
        CesiumGltf::FeatureTexturePropertyView>& properties =
        this->_featureTextureView.getProperties();
    this->_propertyKeys.Reserve(properties.size());
    for (auto propertyView : properties) {
      this->_propertyKeys.Emplace(UTF8_TO_TCHAR(propertyView.first.c_str()));
    }
  }
}

const TArray<FString>& UCesiumFeatureTextureBlueprintLibrary::GetPropertyKeys(
    UPARAM(ref) const FCesiumFeatureTexture& FeatureTexture) {
  return FeatureTexture._propertyKeys;
}

FCesiumFeatureTextureProperty
UCesiumFeatureTextureBlueprintLibrary::FindProperty(
    UPARAM(ref) const FCesiumFeatureTexture& FeatureTexture,
    const FString& PropertyName) {

  const std::unordered_map<std::string, CesiumGltf::FeatureTexturePropertyView>&
      properties = FeatureTexture._featureTextureView.getProperties();

  auto propertyIt = properties.find(TCHAR_TO_UTF8(*PropertyName));
  if (propertyIt == properties.end()) {
    return FCesiumFeatureTextureProperty();
  }

  return FCesiumFeatureTextureProperty(propertyIt->second);
}
