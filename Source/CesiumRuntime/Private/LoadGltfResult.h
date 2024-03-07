// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumEncodedFeaturesMetadata.h"
#include "CesiumMetadataPrimitive.h"
#include "CesiumModelMetadata.h"
#include "CesiumPrimitiveFeatures.h"
#include "CesiumPrimitiveMetadata.h"
#include "CesiumRasterOverlays.h"
#include "CesiumTextureUtility.h"
#include "Chaos/TriangleMeshImplicitObject.h"
#include "Containers/Map.h"
#include "Containers/UnrealString.h"
#include "Math/TransformNonVectorized.h"
#include "StaticMeshResources.h"
#include "Templates/SharedPointer.h"

#include <CesiumGltf/AccessorUtility.h>
#include <CesiumGltf/Material.h>
#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <cstdint>
#include <glm/mat4x4.hpp>
#include <optional>
#include <string>
#include <vector>
#include <unordered_map>

namespace LoadGltfResult {
/**
 * Represents the result of loading a glTF primitive on a game thread.
 * Temporarily holds render data that will be used in the Unreal material, as
 * well as any data that needs to be transferred to the corresponding
 * CesiumGltfPrimitiveComponent after it is created on the main thread.
 */
struct LoadPrimitiveResult {
#pragma region Temporary render data

  /**
   * The render data. This is populated so it can be set on the static mesh
   * created on the main thread.
   */
  TUniquePtr<FStaticMeshRenderData> RenderData = nullptr;

  /**
   * A pointer to the glTF material.
   */
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
  /**
   * A map of feature ID set names to their corresponding texture coordinate
   * indices in the Unreal mesh.
   */
  TMap<FString, uint32_t> FeaturesMetadataTexCoordParameters;

  bool isUnlit = false;

  bool onlyLand = true;
  bool onlyWater = false;

  double waterMaskTranslationX = 0.0;
  double waterMaskTranslationY = 0.0;
  double waterMaskScale = 1.0;

  /**
   * The dimensions of the primitive. Passed to a CesiumGltfPointsComponent for
   * use in computing attenuation.
   */
  glm::vec3 dimensions;

#pragma endregion

#pragma region CesiumGltfPrimitiveComponent data
  const CesiumGltf::Model* pModel = nullptr;
  const CesiumGltf::MeshPrimitive* pMeshPrimitive = nullptr;

  /** Parses EXT_mesh_features from a mesh primitive.*/
  FCesiumPrimitiveFeatures Features{};
  /** Parses EXT_structural_metadata from a mesh primitive.*/
  FCesiumPrimitiveMetadata Metadata{};

  /** Encodes the EXT_mesh_features on a mesh primitive.*/
  CesiumEncodedFeaturesMetadata::EncodedPrimitiveFeatures EncodedFeatures{};
  /** Encodes the EXT_structural_metadata on a mesh primitive.*/
  CesiumEncodedFeaturesMetadata::EncodedPrimitiveMetadata EncodedMetadata{};

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  // For backwards compatibility with CesiumEncodedMetadataComponent.
  FCesiumMetadataPrimitive Metadata_DEPRECATED{};
  std::optional<CesiumEncodedMetadataUtility::EncodedMetadataPrimitive>
      EncodedMetadata_DEPRECATED = std::nullopt;
  PRAGMA_ENABLE_DEPRECATION_WARNINGS

  /**
   * Maps an overlay texture coordinate ID to the index of the corresponding
   * texture coordinates in the mesh's UVs array.
   */
  OverlayTextureCoordinateIDMap overlayTextureCoordinateIDToUVIndex{};

  /**
   * Maps the accessor index in a glTF to its corresponding texture coordinate
   * index in the Unreal mesh. The -1 key is reserved for implicit feature IDs
   * (in other words, the vertex index).
   */
  std::unordered_map<int32_t, uint32_t> GltfToUnrealTexCoordMap;

  /**
   * Maps texture coordinate set indices in a glTF to AccessorViews. This stores
   * accessor views on texture coordinate sets that will be used by feature ID
   * textures or property textures for picking.
   */
  std::unordered_map<int32_t, CesiumGltf::TexCoordAccessorType>
      TexCoordAccessorMap;

  /**
   * The position accessor of the glTF primitive. This is used for computing
   * the UV at a hit location on a primitive, and is safer to access than the
   * mesh's RenderData.
   */
  CesiumGltf::AccessorView<FVector3f> PositionAccessor;

  /**
   * The index accessor of the glTF primitive, if one is specified. This is used
   * for computing the UV at a hit location on a primitive.
   */
  CesiumGltf::IndexAccessorType IndexAccessor;

#pragma endregion
};

/**
 * Represents the result of loading a glTF mesh on a game thread.
 */
struct LoadMeshResult {
  std::vector<LoadPrimitiveResult> primitiveResults{};
};

/**
 * Represents the result of loading a glTF node on a game thread.
 */
struct LoadNodeResult {
  std::optional<LoadMeshResult> meshResult = std::nullopt;
  /**
   * Array of instance transforms, if any.
   */
  std::vector<FTransform> InstanceTransforms;
};

/**
 * Represents the result of loading a glTF model on a game thread.
 * Temporarily holds data that needs to be transferred to the corresponding
 * CesiumGltfComponent after it is created on the main thread.
 */
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
