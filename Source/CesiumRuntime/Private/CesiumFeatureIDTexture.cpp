// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumFeatureIDTexture.h"

#include "CesiumGltf/FeatureIDTexture.h"
#include "CesiumGltf/Model.h"

using namespace CesiumGltf;

FCesiumFeatureIDTexture::FCesiumFeatureIDTexture(
    const Model& model,
    const FeatureIDTexture& featureIDTexture)
    : _featureIDTextureView(model, featureIDTexture),
      _featureTable(model, *this->_featureIDTextureView.getFeatureTable()) {}

const FCesiumMetadataFeatureTable&
UCesiumFeatureIDTextureBlueprintLibrary::GetFeatureTable(
    const FCesiumFeatureIDTexture& featureIDTexture) {
  return featureIDTexture._featureTable;
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