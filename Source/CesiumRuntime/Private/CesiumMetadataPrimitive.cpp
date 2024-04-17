// Copyright 2020-2023 CesiumGS, Inc. and Contributors

PRAGMA_DISABLE_DEPRECATION_WARNINGS

#include "CesiumMetadataPrimitive.h"
#include "CesiumGltf/Model.h"

FCesiumMetadataPrimitive::FCesiumMetadataPrimitive(
    const FCesiumPrimitiveFeatures& PrimitiveFeatures,
    const FCesiumPrimitiveMetadata& PrimitiveMetadata,
    const FCesiumModelMetadata& ModelMetadata)
    : _pPrimitiveFeatures(&PrimitiveFeatures),
      _pPrimitiveMetadata(&PrimitiveMetadata),
      _pModelMetadata(&ModelMetadata) {}

const TArray<FCesiumFeatureIdAttribute>
UCesiumMetadataPrimitiveBlueprintLibrary::GetFeatureIdAttributes(
    UPARAM(ref) const FCesiumMetadataPrimitive& MetadataPrimitive) {
  TArray<FCesiumFeatureIdAttribute> featureIDAttributes;
  if (!MetadataPrimitive._pPrimitiveFeatures) {
    return featureIDAttributes;
  }

  const TArray<FCesiumFeatureIdSet> featureIDSets =
      UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSetsOfType(
          *MetadataPrimitive._pPrimitiveFeatures,
          ECesiumFeatureIdSetType::Attribute);

  featureIDAttributes.Reserve(featureIDSets.Num());
  for (const FCesiumFeatureIdSet& featureIDSet : featureIDSets) {
    featureIDAttributes.Add(
        UCesiumFeatureIdSetBlueprintLibrary::GetAsFeatureIDAttribute(
            featureIDSet));
  }

  return featureIDAttributes;
}

const TArray<FCesiumFeatureIdTexture>
UCesiumMetadataPrimitiveBlueprintLibrary::GetFeatureIdTextures(
    UPARAM(ref) const FCesiumMetadataPrimitive& MetadataPrimitive) {
  TArray<FCesiumFeatureIdTexture> featureIDTextures;
  if (!MetadataPrimitive._pPrimitiveFeatures) {
    return featureIDTextures;
  }

  const TArray<FCesiumFeatureIdSet> featureIDSets =
      UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSetsOfType(
          *MetadataPrimitive._pPrimitiveFeatures,
          ECesiumFeatureIdSetType::Texture);

  featureIDTextures.Reserve(featureIDSets.Num());
  for (const FCesiumFeatureIdSet& featureIDSet : featureIDSets) {
    featureIDTextures.Add(
        UCesiumFeatureIdSetBlueprintLibrary::GetAsFeatureIDTexture(
            featureIDSet));
  }

  return featureIDTextures;
}

const TArray<FString>
UCesiumMetadataPrimitiveBlueprintLibrary::GetFeatureTextureNames(
    UPARAM(ref) const FCesiumMetadataPrimitive& MetadataPrimitive) {
  TArray<FString> propertyTextureNames;
  if (!MetadataPrimitive._pPrimitiveMetadata ||
      !MetadataPrimitive._pModelMetadata) {
    return TArray<FString>();
  }

  const TArray<int64>& propertyTextureIndices =
      UCesiumPrimitiveMetadataBlueprintLibrary::GetPropertyTextureIndices(
          *MetadataPrimitive._pPrimitiveMetadata);

  const TArray<FCesiumPropertyTexture> propertyTextures =
      UCesiumModelMetadataBlueprintLibrary::GetPropertyTexturesAtIndices(
          *MetadataPrimitive._pModelMetadata,
          propertyTextureIndices);

  propertyTextureNames.Reserve(propertyTextures.Num());
  for (auto propertyTexture : propertyTextures) {
    propertyTextureNames.Add(
        UCesiumPropertyTextureBlueprintLibrary::GetPropertyTextureName(
            propertyTexture));
  }

  return propertyTextureNames;
}

int64 UCesiumMetadataPrimitiveBlueprintLibrary::GetFirstVertexIDFromFaceID(
    UPARAM(ref) const FCesiumMetadataPrimitive& MetadataPrimitive,
    int64 FaceID) {
  if (!MetadataPrimitive._pPrimitiveFeatures) {
    return -1;
  }

  return UCesiumPrimitiveFeaturesBlueprintLibrary::GetFirstVertexFromFace(
      *MetadataPrimitive._pPrimitiveFeatures,
      FaceID);
}

PRAGMA_ENABLE_DEPRECATION_WARNINGS
