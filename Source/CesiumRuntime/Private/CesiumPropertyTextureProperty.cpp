// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumPropertyTextureProperty.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumMetadataConversions.h"

#include <cstdint>
#include <limits>

// clang-format off
int64 UCesiumPropertyTexturePropertyBlueprintLibrary::GetTextureCoordinateIndex(
    const UPrimitiveComponent* Component,
    UPARAM(ref) const FCesiumPropertyTextureProperty& Property) {
  return -1;
  /*const UCesiumGltfPrimitiveComponent* pPrimitive =
      Cast<UCesiumGltfPrimitiveComponent>(Component);
  if (!pPrimitive) {
    return -1;
  }

  auto textureCoordinateIndexIt = pPrimitive->textureCoordinateMap.find(
      Property._pPropertyView->getTexCoordSetIndex());
  if (textureCoordinateIndexIt == pPrimitive->textureCoordinateMap.end()) {
    return -1;
  }

  return textureCoordinateIndexIt->second;*/
}

int64 UCesiumPropertyTexturePropertyBlueprintLibrary::GetComponentCount(
    UPARAM(ref) const FCesiumPropertyTextureProperty& property) {
  return 0;
  // property._pPropertyView->getCount();
}
//
// bool UCesiumPropertyTexturePropertyBlueprintLibrary::IsNormalized(
//    UPARAM(ref) const FCesiumFeatureTextureProperty& property) {
//  return property._pPropertyView->isNormalized();
//}
//
// FString UCesiumPropertyTexturePropertyBlueprintLibrary::GetSwizzle(
//    UPARAM(ref) const FCesiumFeatureTextureProperty& property) {
//  return UTF8_TO_TCHAR(property._pProperty->getSwizzle().c_str());
//}
//
// FCesiumIntegerColor UCesiumPropertyTexturePropertyBlueprintLibrary::
//    GetIntegerColorFromTextureCoordinates(
//        UPARAM(ref) const FCesiumFeatureTextureProperty& property,
//        float u,
//        float v) {
//  switch (property._pProperty->getPropertyType()) {
//  case CesiumGltf::PropertyTexturePropertyComponentType::Uint8: {
//    CesiumGltf::PropertyTexturePropertyValue<uint8_t> propertyValue =
//        property._pProperty->getProperty<uint8_t>(u, v);
//    return {
//        CesiumMetadataConversions<int32, uint8_t>::convert(
//            propertyValue.components[0],
//            0),
//        CesiumMetadataConversions<int32, uint8_t>::convert(
//            propertyValue.components[1],
//            0),
//        CesiumMetadataConversions<int32, uint8_t>::convert(
//            propertyValue.components[2],
//            0),
//        CesiumMetadataConversions<int32, uint8_t>::convert(
//            propertyValue.components[3],
//            0)};
//  } break;
//  default:
//    return {0, 0, 0, 0};
//  }
//}
//
// FCesiumFloatColor UCesiumPropertyTexturePropertyBlueprintLibrary::
//    GetFloatColorFromTextureCoordinates(
//        UPARAM(ref) const FCesiumFeatureTextureProperty& property,
//        float u,
//        float v) {
//  switch (property._pProperty->getPropertyType()) {
//  case CesiumGltf::FeatureTexturePropertyComponentType::Uint8: {
//    CesiumGltf::FeatureTexturePropertyValue<uint8_t> propertyValue =
//        property._pProperty->getProperty<uint8_t>(u, v);
//
//    float normalizationDenominator = 1.0f;
//    if (property._pProperty->isNormalized()) {
//      normalizationDenominator = std::numeric_limits<uint8_t>::max();
//    }
//
//    return {
//        CesiumMetadataConversions<float, uint8_t>::convert(
//            propertyValue.components[0],
//            0.0f) /
//            normalizationDenominator,
//        CesiumMetadataConversions<float, uint8_t>::convert(
//            propertyValue.components[1],
//            0.0f) /
//            normalizationDenominator,
//        CesiumMetadataConversions<float, uint8_t>::convert(
//            propertyValue.components[2],
//            0.0f) /
//            normalizationDenominator,
//        CesiumMetadataConversions<float, uint8_t>::convert(
//            propertyValue.components[3],
//            0.0f) /
//            normalizationDenominator};
//  } break;
//  default:
//    return {0.0f, 0.0f, 0.0f, 0.0f};
//  }
//}

// clang-format on
