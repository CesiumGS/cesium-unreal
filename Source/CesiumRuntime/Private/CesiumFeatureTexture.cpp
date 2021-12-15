
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
    this->_properties.Reserve(properties.size());
    for (const auto& propertyView : properties) {
      this->_properties.Add(
          FString(UTF8_TO_TCHAR(propertyView.first.c_str())),
          FCesiumFeatureTextureProperty(propertyView.second));
    }
  }
}

const TMap<FString, FCesiumFeatureTextureProperty>&
UCesiumFeatureTextureBlueprintLibrary::GetProperties(
    UPARAM(ref) const FCesiumFeatureTexture& featureTexture) {
  return featureTexture._properties;
}
