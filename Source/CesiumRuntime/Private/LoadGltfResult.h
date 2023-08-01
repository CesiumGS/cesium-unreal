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

  CesiumEncodedFeaturesMetadata::EncodedPrimitiveFeatures EncodedFeatures{};
  CesiumEncodedFeaturesMetadata::EncodedPrimitiveMetadata EncodedMetadata{};
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
  std::unordered_map<uint32_t, uint32_t> textureCoordinateMap;

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
  CesiumEncodedFeaturesMetadata::EncodedModelMetadata EncodedModelMetadata{};
};
} // namespace LoadGltfResult
