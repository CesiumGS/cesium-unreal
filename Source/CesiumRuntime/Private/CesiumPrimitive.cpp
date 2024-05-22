// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumPrimitive.h"

void CesiumPrimitiveData::destroy() {
  this->Features = FCesiumPrimitiveFeatures();
  this->Metadata = FCesiumPrimitiveMetadata();
  this->EncodedFeatures =
      CesiumEncodedFeaturesMetadata::EncodedPrimitiveFeatures();
  this->EncodedMetadata =
      CesiumEncodedFeaturesMetadata::EncodedPrimitiveMetadata();

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  this->Metadata_DEPRECATED = FCesiumMetadataPrimitive();
  this->EncodedMetadata_DEPRECATED.reset();
  PRAGMA_ENABLE_DEPRECATION_WARNINGS

  this->pTilesetActor = nullptr;
  this->pModel = nullptr;
  this->pMeshPrimitive = nullptr;

  std::unordered_map<int32_t, uint32_t> emptyTexCoordMap;
  this->GltfToUnrealTexCoordMap.swap(emptyTexCoordMap);

  std::unordered_map<int32_t, CesiumGltf::TexCoordAccessorType>
      emptyAccessorMap;
  this->TexCoordAccessorMap.swap(emptyAccessorMap);
}

