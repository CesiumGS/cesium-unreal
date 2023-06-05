// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumFeatureIdTexture.h"

#include "CesiumGltf/ExtensionExtMeshFeaturesFeatureIdTexture.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltfPrimitiveComponent.h"

using namespace CesiumGltf;
using namespace CesiumGltf::MeshFeatures;

FCesiumFeatureIDTexture::FCesiumFeatureIDTexture(
    const Model& model,
    const ExtensionExtMeshFeaturesFeatureIdTexture& featureIdTexture)
    : _featureIDTextureView(model, featureIdTexture) {}

int64 UCesiumFeatureIDTextureBlueprintLibrary::GetTextureCoordinateIndex(
    const UPrimitiveComponent* component,
    const FCesiumFeatureIDTexture& featureIDTexture) {
  const UCesiumGltfPrimitiveComponent* pPrimitive =
      Cast<UCesiumGltfPrimitiveComponent>(component);
  if (!pPrimitive) {
    return -1;
  }

  if (featureIDTexture._featureIDTextureView.status() !=
      FeatureIdTextureViewStatus::Valid) {
    return -1;
  }

  auto textureCoordinateIndexIt = pPrimitive->textureCoordinateMap.find(
      featureIDTexture._featureIDTextureView.getTexCoordSetIndex());
  if (textureCoordinateIndexIt == pPrimitive->textureCoordinateMap.end()) {
    return -1;
  }

  return textureCoordinateIndexIt->second;
}

int64 UCesiumFeatureIDTextureBlueprintLibrary::
    GetFeatureIDForTextureCoordinates(
        const FCesiumFeatureIDTexture& featureIDTexture,
        float u,
        float v) {
  if (featureIDTexture._featureIDTextureView.status() !=
      FeatureIdTextureViewStatus::Valid) {
    return -1;
  }

  return featureIDTexture._featureIDTextureView.getFeatureId(u, v);
}
