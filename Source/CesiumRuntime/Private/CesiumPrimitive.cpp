// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumPrimitive.h"

FVector3f scalePositionForUnreal(const FVector3f& position) {
  FVector3f result;
  result.X = position.X * CesiumPrimitiveData::positionScaleFactor;
  result.Y = -position.Y * CesiumPrimitiveData::positionScaleFactor;
  result.Z = position.Z * CesiumPrimitiveData::positionScaleFactor;
  return result;
}

void CesiumPrimitiveData::destroy() {
  this->features = FCesiumPrimitiveFeatures();
  this->metadata = FCesiumPrimitiveMetadata();
  this->encodedFeatures = EncodedFeaturesMetadata::EncodedPrimitiveFeatures();
  this->encodedMetadata = EncodedFeaturesMetadata::EncodedPrimitiveMetadata();

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  this->metadata_DEPRECATED = FCesiumMetadataPrimitive();
  this->encodedMetadata_DEPRECATED.reset();
  PRAGMA_ENABLE_DEPRECATION_WARNINGS

  this->pTilesetActor = nullptr;
  this->pMeshPrimitive = nullptr;

  std::unordered_map<int32_t, uint32_t> emptyTexCoordMap;
  this->gltfToUnrealTexCoordMap.swap(emptyTexCoordMap);

  std::unordered_map<int32_t, CesiumGltf::TexCoordAccessorType>
      emptyAccessorMap;
  this->texCoordAccessorMap.swap(emptyAccessorMap);
}

const CesiumGltf::MeshPrimitive* ICesiumPrimitive::GetMeshPrimitive() const {
  return this->getPrimitiveData().pMeshPrimitive;
}

const FCesiumPrimitiveFeatures& ICesiumPrimitive::GetPrimitiveFeatures() const {
  return this->getPrimitiveData().features;
}

const FCesiumPrimitiveMetadata& ICesiumPrimitive::GetPrimitiveMetadata() const {
  return this->getPrimitiveData().metadata;
}
