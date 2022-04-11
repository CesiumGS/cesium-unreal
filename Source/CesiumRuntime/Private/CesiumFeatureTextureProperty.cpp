// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumFeatureTextureProperty.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumMetadataConversions.h"

#include <cstdint>
#include <limits>

int64 UCesiumFeatureTexturePropertyBlueprintLibrary::GetTextureCoordinateIndex(
    const UPrimitiveComponent* component,
    UPARAM(ref) const FCesiumFeatureTextureProperty& property) {
  const UCesiumGltfPrimitiveComponent* pPrimitive =
      Cast<UCesiumGltfPrimitiveComponent>(component);
  if (!pPrimitive) {
    return 0;
  }

  auto textureCoordinateIndexIt = pPrimitive->textureCoordinateMap.find(
      property._pProperty->getTextureCoordinateAttributeId());
  if (textureCoordinateIndexIt == pPrimitive->textureCoordinateMap.end()) {
    return 0;
  }

  return textureCoordinateIndexIt->second;
}

int64 UCesiumFeatureTexturePropertyBlueprintLibrary::GetComponentCount(
    UPARAM(ref) const FCesiumFeatureTextureProperty& property) {
  return property._pProperty->getComponentCount();
}

bool UCesiumFeatureTexturePropertyBlueprintLibrary::IsNormalized(
    UPARAM(ref) const FCesiumFeatureTextureProperty& property) {
  return property._pProperty->isNormalized();
}

FString UCesiumFeatureTexturePropertyBlueprintLibrary::GetSwizzle(
    UPARAM(ref) const FCesiumFeatureTextureProperty& property) {
  return UTF8_TO_TCHAR(property._pProperty->getSwizzle().c_str());
}

FCesiumIntegerColor UCesiumFeatureTexturePropertyBlueprintLibrary::
    GetIntegerColorFromTextureCoordinates(
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
  } break;
  default:
    return {0, 0, 0, 0};
  }
}

FCesiumFloatColor UCesiumFeatureTexturePropertyBlueprintLibrary::
    GetFloatColorFromTextureCoordinates(
        UPARAM(ref) const FCesiumFeatureTextureProperty& property,
        float u,
        float v) {
  switch (property._pProperty->getPropertyType()) {
  case CesiumGltf::FeatureTexturePropertyComponentType::Uint8: {
    CesiumGltf::FeatureTexturePropertyValue<uint8_t> propertyValue =
        property._pProperty->getProperty<uint8_t>(u, v);

    float normalizationDenominator = 1.0f;
    if (property._pProperty->isNormalized()) {
      normalizationDenominator = std::numeric_limits<uint8_t>::max();
    }

    return {
        CesiumMetadataConversions<float, uint8_t>::convert(
            propertyValue.components[0],
            0.0f) /
            normalizationDenominator,
        CesiumMetadataConversions<float, uint8_t>::convert(
            propertyValue.components[1],
            0.0f) /
            normalizationDenominator,
        CesiumMetadataConversions<float, uint8_t>::convert(
            propertyValue.components[2],
            0.0f) /
            normalizationDenominator,
        CesiumMetadataConversions<float, uint8_t>::convert(
            propertyValue.components[3],
            0.0f) /
            normalizationDenominator};
  } break;
  default:
    return {0.0f, 0.0f, 0.0f, 0.0f};
  }
}
