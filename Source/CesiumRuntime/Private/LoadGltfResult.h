// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumEncodedFeaturesMetadata.h"
#include "CesiumGltf/Material.h"
#include "CesiumGltf/MeshPrimitive.h"
#include "CesiumGltf/Model.h"
#include "CesiumMetadataPrimitive.h"
#include "CesiumModelMetadata.h"
#include "CesiumPrimitiveFeatures.h"
#include "CesiumPrimitiveMetadata.h"
#include "CesiumRasterOverlays.h"
#include "CesiumTextureUtility.h"
#include "Chaos/TriangleMeshImplicitObject.h"
#include "Containers/Map.h"
#include "Containers/UnrealString.h"
#include "StaticMeshResources.h"
#include "Templates/SharedPointer.h"
#include <cstdint>
#include <glm/mat4x4.hpp>
#include <optional>
#include <string>
#include <unordered_map>

// TODO: internal documentation
namespace LoadGltfResult {
struct LoadPrimitiveResult {
  // Parses EXT_mesh_features from a mesh primitive.
  FCesiumPrimitiveFeatures Features{};
  // Parses EXT_structural_metadata from a mesh primitive.
  FCesiumPrimitiveMetadata Metadata{};

  // Encodes the EXT_mesh_features on a mesh primitive.
  CesiumEncodedFeaturesMetadata::EncodedPrimitiveFeatures EncodedFeatures{};
  // Encodes the EXT_structural_metadata on a mesh primitive.
  CesiumEncodedFeaturesMetadata::EncodedPrimitiveMetadata EncodedMetadata{};

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  // For backwards compatibility with CesiumEncodedMetadataComponent.
  FCesiumMetadataPrimitive Metadata_DEPRECATED{};
  std::optional<CesiumEncodedMetadataUtility::EncodedMetadataPrimitive>
      EncodedMetadata_DEPRECATED = std::nullopt;
  PRAGMA_ENABLE_DEPRECATION_WARNINGS

  // A map of feature ID set names to their corresponding texture coordinate
  // indices in the Unreal mesh.
  TMap<FString, uint32_t> featuresMetadataTexCoordParameters;

  TUniquePtr<FStaticMeshRenderData> RenderData = nullptr;
  const CesiumGltf::Model* pModel = nullptr;
  const CesiumGltf::MeshPrimitive* pMeshPrimitive = nullptr;
  const CesiumGltf::Material* pMaterial = nullptr;
  glm::dmat4x4 transform{1.0};
  TSharedPtr<Chaos::FTriangleMeshImplicitObject, ESPMode::ThreadSafe>
      pCollisionMesh = nullptr;
  std::string name{};

  TUniquePtr<CesiumTextureUtility::LoadedTextureResult> baseColorTexture;
  TUniquePtr<CesiumTextureUtility::LoadedTextureResult>
      metallicRoughnessTexture;
  TUniquePtr<CesiumTextureUtility::LoadedTextureResult> normalTexture;
  TUniquePtr<CesiumTextureUtility::LoadedTextureResult> emissiveTexture;
  TUniquePtr<CesiumTextureUtility::LoadedTextureResult> occlusionTexture;
  TUniquePtr<CesiumTextureUtility::LoadedTextureResult> waterMaskTexture;
  std::unordered_map<std::string, uint32_t> textureCoordinateParameters;

  bool isUnlit = false;

  bool onlyLand = true;
  bool onlyWater = false;

  double waterMaskTranslationX = 0.0;
  double waterMaskTranslationY = 0.0;
  double waterMaskScale = 1.0;

  OverlayTextureCoordinateIDMap overlayTextureCoordinateIDToUVIndex{};
  // Maps the accessor index in a glTF to its corresponding texture coordinate
  // index in the Unreal mesh.
  // The -1 key is reserved for implicit feature IDs (in other words, the vertex
  // index).
  std::unordered_map<int32_t, uint32_t> textureCoordinateMap;

  glm::vec3 dimensions;
};

struct LoadMeshResult {
  std::vector<LoadPrimitiveResult> primitiveResults{};
};

struct LoadNodeResult {
  std::optional<LoadMeshResult> meshResult = std::nullopt;
};

struct LoadModelResult {
  std::vector<LoadNodeResult> nodeResults{};
  // Parses the root EXT_structural_metadata extension.
  FCesiumModelMetadata Metadata{};
  // Encodes the EXT_structural_metadata on a glTF model.
  CesiumEncodedFeaturesMetadata::EncodedModelMetadata EncodedMetadata{};

  // For backwards compatibility with CesiumEncodedMetadataComponent.
  std::optional<CesiumEncodedMetadataUtility::EncodedMetadata>
      EncodedMetadata_DEPRECATED{};
};
} // namespace LoadGltfResult
