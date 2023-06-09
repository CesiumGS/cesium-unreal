// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumMetadataPrimitive.h"
#include "CesiumGltf/ExtensionMeshPrimitiveExtFeatureMetadata.h"
#include "CesiumGltf/Model.h"

FCesiumMetadataPrimitive::FCesiumMetadataPrimitive(
    const CesiumGltf::Model& model,
    const CesiumGltf::MeshPrimitive& primitive,
    const CesiumGltf::ExtensionMeshPrimitiveExtFeatureMetadata& metadata,
    const FCesiumPrimitiveFeatures& primitiveFeatures)
    : _pPrimitiveFeatures(&primitiveFeatures), _featureTextureNames() {
  this->_featureTextureNames.Reserve(metadata.featureTextures.size());
  for (const std::string& featureTextureName : metadata.featureTextures) {
    this->_featureTextureNames.Emplace(
        UTF8_TO_TCHAR(featureTextureName.c_str()));
  }
}

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
          ECesiumFeatureIdType::Attribute);

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
          ECesiumFeatureIdType::Texture);

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
  if (!MetadataPrimitive._pPrimitiveMetadata) {
    return;
  }

  const TArray<int64>& propertyTextureIndices =
      UCesiumPrimitiveMetadataBlueprintLibrary::GetPropertyTextureIndices(
          *MetadataPrimitive._pPrimitiveMetadata);

  propertyTextureNames.Reserve(propertyTextureIndices.Num());
  for (const int64 index : propertyTextureIndices) {
    // TODO
    /*propertyTextureNames.Add(
        UCesiumFeatureIdSetBlueprintLibrary::GetAsFeatureIDTexture(
            featureIDSet));*/
  }

  return propertyTextureNames;
}

int64 UCesiumMetadataPrimitiveBlueprintLibrary::GetFirstVertexIDFromFaceID(
    UPARAM(ref) const FCesiumMetadataPrimitive& MetadataPrimitive,
    int64 FaceID) {
  return UCesiumPrimitiveFeaturesBlueprintLibrary::GetFirstVertexFromFace(
      *MetadataPrimitive._pPrimitiveFeatures,
      FaceID);
}
