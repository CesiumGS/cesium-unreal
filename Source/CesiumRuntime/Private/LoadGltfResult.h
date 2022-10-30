// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumEncodedMetadataUtility.h"
#include "CesiumGltf/Material.h"
#include "CesiumGltf/MeshPrimitive.h"
#include "CesiumGltf/Model.h"
#include "CesiumMetadataModel.h"
#include "CesiumMetadataPrimitive.h"
#include "CesiumPhysicsUtility.h"
#include "CesiumRasterOverlays.h"
#include "CesiumTextureUtility.h"
#include "Containers/Map.h"
#include "Containers/UnrealString.h"
#include "StaticMeshResources.h"
#include "Templates/SharedPointer.h"
#include <cstdint>
#include <glm/mat4x4.hpp>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace LoadGltfResult {
struct LoadPrimitiveResult {
  /**
   * @brief Accessor for primitive-level metadata.
   */
  FCesiumMetadataPrimitive Metadata{};

  /**
   * @brief Render resources for primitive-level metadata - used for GPU
   * metadata styling.
   */
  CesiumEncodedMetadataUtility::EncodedMetadataPrimitive EncodedMetadata{};

  /**
   * @brief Texture coordinate set paramaters needed for metadata styling.
   * These may contain feature ID attributes packed into texture coordinates or
   * texture coordinates corresponding to metadata textures.
   *
   * This maps the uniform parameter name to the index of the corresponding
   * texture coordinate set within the static mesh.
   *
   * See CesiumGltfComponent::SetMetadataParameterValues for implementation
   * details and documentation on how these paramater names are constructed.
   */
  TMap<FString, uint32_t> metadataTextureCoordinateParameters;

  /**
   * @brief Render resource for this primitive's static mesh.
   */
  TUniquePtr<FStaticMeshRenderData> RenderData = nullptr;

  // Stashed pointers to sections of the in-memory glTF corresponding to this
  // primitive.
  // TODO: Investigate if saving these between worker thread loading and main
  // thread loading is completely safe.

  const CesiumGltf::Model* pModel = nullptr;
  const CesiumGltf::MeshPrimitive* pMeshPrimitive = nullptr;
  const CesiumGltf::Material* pMaterial = nullptr;

  /**
   * @brief Double-precision transform of this primitive in ECEF.
   */
  glm::dmat4x4 transform{1.0};

  /**
   * @brief Cooked physics mesh buffer to cache, corresponding to this
   * primitive.
   *
   * Might be PhysX or Chaos data.
   */
  std::vector<std::byte> cookedPhysicsMesh{};

#if PHYSICS_INTERFACE_PHYSX
  /**
   * @brief Created PhysX mesh for this primitive.
   */
  CesiumPhysicsUtility::CesiumPhysxMesh physxMesh{};
#else

  /**
   * @brief Created Chaos mesh for this primitive.
   */
  TSharedPtr<Chaos::FTriangleMeshImplicitObject, ESPMode::ThreadSafe>
      pCollisionMesh = nullptr;
#endif

  /**
   * @brief The name string that will be used for this primitive component
   * within Unreal.
   */
  std::string name{};

  // Render resources for the base glTF material textures.

  TUniquePtr<CesiumTextureUtility::LoadedTextureResult> baseColorTexture;
  TUniquePtr<CesiumTextureUtility::LoadedTextureResult>
      metallicRoughnessTexture;
  TUniquePtr<CesiumTextureUtility::LoadedTextureResult> normalTexture;
  TUniquePtr<CesiumTextureUtility::LoadedTextureResult> emissiveTexture;
  TUniquePtr<CesiumTextureUtility::LoadedTextureResult> occlusionTexture;

  // Render resources and UV scaling / translation info for the water mask.
  // Only applies to glTFs created from quantized mesh tiles with the water
  // mask extension.
  // TODO: wrap these into some sort of struct.

  TUniquePtr<CesiumTextureUtility::LoadedTextureResult> waterMaskTexture;

  bool onlyLand = true;
  bool onlyWater = false;

  double waterMaskTranslationX = 0.0;
  double waterMaskTranslationY = 0.0;
  double waterMaskScale = 1.0;

  /**
   * @brief Texture coordinate set paramaters needed for the base glTF material
   * textures and the water mask texture.
   *
   * This maps the uniform parameter name to the index of the corresponding
   * texture coordinate set within the static mesh.
   */
  std::unordered_map<std::string, uint32_t> textureCoordinateParameters;

  /**
   * @brief Texture coordinate sets needed for attached raster overlays.
   *
   * This maps the overlay ID (e.g., overlay 0 is the glTF primitive attribute
   * with the name _CESIUMOVERLAY_0) to the index of the corresponding texture
   * coordinate set within the final static mesh.
   */
  OverlayTextureCoordinateIDMap overlayTextureCoordinateIDToUVIndex{};

  /**
   * @brief Texture coordinate sets needed for base glTF material textures,
   * metadata textures, metadata attributes (packed into UV coordinates), the
   * water mask, and raster overlays.
   *
   * This maps the index of the wanted glTF attribute within
   * meshPrimitive.attributes list to the index of the corresponding texture
   * coordinate set within the final static mesh.
   */
  std::unordered_map<uint32_t, uint32_t> textureCoordinateMap;
};

struct LoadMeshResult {
  std::vector<LoadPrimitiveResult> primitiveResults{};
};

struct LoadNodeResult {
  std::optional<LoadMeshResult> meshResult = std::nullopt;
};

struct LoadModelResult {
  std::vector<LoadNodeResult> nodeResults{};

  /**
   * @brief Accessor for model-level metadata.
   */
  FCesiumMetadataModel Metadata{};

  /**
   * @brief Render resources for model-level metadata - used for GPU metadata
   * styling.
   */
  CesiumEncodedMetadataUtility::EncodedMetadata EncodedMetadata{};
};
} // namespace LoadGltfResult
