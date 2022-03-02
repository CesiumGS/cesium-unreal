// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumFeatureIDTexture.h"

#include "CesiumGltf/FeatureIDTexture.h"
#include "CesiumGltf/Model.h"

using namespace CesiumGltf;

FCesiumFeatureIDTexture::FCesiumFeatureIDTexture(
    const Model& model,
    const FeatureIDTexture& featureIDTexture)
    : _featureIDTextureView(model, featureIDTexture),
      _featureTableName(UTF8_TO_TCHAR(
          this->_featureIDTextureView.getFeatureTableName().c_str())) {}

const FString& UCesiumFeatureIDTextureBlueprintLibrary::GetFeatureTableName(
    const FCesiumFeatureIDTexture& featureIDTexture) {
  return featureIDTexture._featureTableName;
}

int64 UCesiumFeatureIDTextureBlueprintLibrary::GetTextureCoordinateIndex(
    const FCesiumFeatureIDTexture& featureIDTexture) {
  return featureIDTexture._featureIDTextureView.getTextureCoordinateIndex();
}

int64 UCesiumFeatureIDTextureBlueprintLibrary::
    GetFeatureIDForTextureCoordinates(
        const FCesiumFeatureIDTexture& featureIDTexture,
        float u,
        float v) {
  return featureIDTexture._featureIDTextureView.getFeatureID(u, v);
}
