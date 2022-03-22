// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumFeatureIdTexture.h"

#include "CesiumGltf/FeatureIDTexture.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltfPrimitiveComponent.h"

using namespace CesiumGltf;

FCesiumFeatureIdTexture::FCesiumFeatureIdTexture(
    const Model& model,
    const FeatureIDTexture& featureIdTexture)
    : _featureIdTextureView(model, featureIdTexture),
      _featureTableName(UTF8_TO_TCHAR(
          this->_featureIdTextureView.getFeatureTableName().c_str())) {}

const FString& UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureTableName(
    const FCesiumFeatureIdTexture& featureIdTexture) {
  return featureIdTexture._featureTableName;
}

int64 UCesiumFeatureIdTextureBlueprintLibrary::GetTextureCoordinateIndex(
    const UPrimitiveComponent* component,
    const FCesiumFeatureIdTexture& featureIdTexture) {
  const UCesiumGltfPrimitiveComponent* pPrimitive =
      Cast<UCesiumGltfPrimitiveComponent>(component);
  if (!pPrimitive) {
    return 0;
  }

  auto textureCoordinateIndexIt = pPrimitive->textureCoordinateMap.find(
      featureIdTexture._featureIdTextureView.getTextureCoordinateAttributeId());
  if (textureCoordinateIndexIt == pPrimitive->textureCoordinateMap.end()) {
    return 0;
  }

  return textureCoordinateIndexIt->second;
}

int64 UCesiumFeatureIdTextureBlueprintLibrary::
    GetFeatureIdForTextureCoordinates(
        const FCesiumFeatureIdTexture& featureIdTexture,
        float u,
        float v) {
  return featureIdTexture._featureIdTextureView.getFeatureID(u, v);
}
