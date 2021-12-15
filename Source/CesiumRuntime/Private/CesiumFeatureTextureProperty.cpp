
#include "CesiumFeatureTextureProperty.h"
#include "CesiumMetadataConversions.h"

#include <cstdint>

int64 UCesiumFeatureTexturePropertyBlueprintLibrary::GetTextureCoordinateIndex(
    UPARAM(ref) const FCesiumFeatureTextureProperty& property) {
  return property._pProperty->getTextureCoordinateIndex();
}

int64 UCesiumFeatureTexturePropertyBlueprintLibrary::GetComponentCount(
    UPARAM(ref) const FCesiumFeatureTextureProperty& property) {
  return property._pProperty->getComponentCount();
}

FCesiumIntegerColor 
UCesiumFeatureTexturePropertyBlueprintLibrary::GetIntegerColorFromTextureCoordinates(
    UPARAM(ref) const FCesiumFeatureTextureProperty& property,
    float u,
    float v) {
  switch (property._pProperty->getPropertyType()) {
  case CesiumGltf::FeatureTexturePropertyComponentType::Uint8: {
      CesiumGltf::FeatureTexturePropertyValue<uint8_t> propertyValue =
          property._pProperty->getProperty<uint8_t>(u, v);
      return {
          CesiumMetadataConversions<int32, uint8_t>::convert(
            propertyValue.components[0], 
            0),
          CesiumMetadataConversions<int32, uint8_t>::convert(
            propertyValue.components[1], 
            0),
          CesiumMetadataConversions<int32, uint8_t>::convert(
            propertyValue.components[2], 
            0),
          CesiumMetadataConversions<int32, uint8_t>::convert(
            propertyValue.components[3], 
            0)};
    }
    break;
  default:
    return { 0, 0, 0, 0 };
  }
}

FCesiumFloatColor 
UCesiumFeatureTexturePropertyBlueprintLibrary::GetFloatColorFromTextureCoordinates(
    UPARAM(ref) const FCesiumFeatureTextureProperty& property,
    float u,
    float v) {
  switch (property._pProperty->getPropertyType()) {
  case CesiumGltf::FeatureTexturePropertyComponentType::Uint8:
    CesiumGltf::FeatureTexturePropertyValue<uint8_t> propertyValue =
        property._pProperty->getProperty<uint8_t>(u, v);
    return {
        CesiumMetadataConversions<float, uint8_t>::convert(
          propertyValue.components[0],
          0.0f),
        CesiumMetadataConversions<float, uint8_t>::convert(
          propertyValue.components[1], 
          0.0f),
        CesiumMetadataConversions<float, uint8_t>::convert(
          propertyValue.components[2], 
          0.0f),
        CesiumMetadataConversions<float, uint8_t>::convert(
          propertyValue.components[3], 
          0.0f)};
    break;
  default:
    return { 0.0f, 0.0f, 0.0f, 0.0f };
  }
}
