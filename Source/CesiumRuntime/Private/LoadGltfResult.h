// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumEncodedMetadataUtility.h"
#include "CesiumGltf/ExtensionModelMaxarMeshVariants.h"
#include "CesiumGltf/ExtensionNodeMaxarMeshVariants.h"
#include "CesiumGltf/Material.h"
#include "CesiumGltf/MeshPrimitive.h"
#include "CesiumGltf/Model.h"
#include "CesiumMetadataModel.h"
#include "CesiumMetadataPrimitive.h"
#include "CesiumRasterOverlays.h"
#include "CesiumTextureUtility.h"
#include "Containers/Map.h"
#include "Containers/UnrealString.h"
#include "StaticMeshResources.h"
#include "Templates/SharedPointer.h"
#include <cstdint>
#include <glm/mat4x4.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#if PHYSICS_INTERFACE_PHYSX
#include "IPhysXCooking.h"
#include "PhysicsEngine/BodySetup.h"
#include <PxTriangleMesh.h>

struct PxTriangleMeshDeleter {
  void operator()(PxTriangleMesh* pCollisionMesh) {
    if (pCollisionMesh) {
      pCollisionMesh->release();
    }
  }
};
#else
#include "Chaos/TriangleMeshImplicitObject.h"
#endif

// TODO: internal documentation
namespace LoadGltfResult {
struct LoadPrimitiveResult {
  FCesiumMetadataPrimitive Metadata{};
  CesiumEncodedMetadataUtility::EncodedMetadataPrimitive EncodedMetadata{};
  TMap<FString, uint32_t> metadataTextureCoordinateParameters;
  TUniquePtr<FStaticMeshRenderData> RenderData = nullptr;
  const CesiumGltf::Model* pModel = nullptr;
  const CesiumGltf::MeshPrimitive* pMeshPrimitive = nullptr;
  const CesiumGltf::Material* pMaterial = nullptr;
  glm::dmat4x4 transform{1.0};
#if PHYSICS_INTERFACE_PHYSX
  TUniquePtr<PxTriangleMesh, PxTriangleMeshDeleter> pCollisionMesh = nullptr;
  FBodySetupUVInfo uvInfo{};
#else
  TSharedPtr<Chaos::FTriangleMeshImplicitObject, ESPMode::ThreadSafe>
      pCollisionMesh = nullptr;
#endif
  std::string name{};

  TUniquePtr<CesiumTextureUtility::LoadedTextureResult> baseColorTexture =
      nullptr;
  TUniquePtr<CesiumTextureUtility::LoadedTextureResult>
      metallicRoughnessTexture = nullptr;
  TUniquePtr<CesiumTextureUtility::LoadedTextureResult> normalTexture = nullptr;
  TUniquePtr<CesiumTextureUtility::LoadedTextureResult> emissiveTexture =
      nullptr;
  TUniquePtr<CesiumTextureUtility::LoadedTextureResult> occlusionTexture =
      nullptr;
  TUniquePtr<CesiumTextureUtility::LoadedTextureResult> waterMaskTexture =
      nullptr;
  std::unordered_map<std::string, uint32_t> textureCoordinateParameters;

  bool onlyLand = true;
  bool onlyWater = false;

  double waterMaskTranslationX = 0.0;
  double waterMaskTranslationY = 0.0;
  double waterMaskScale = 1.0;

  OverlayTextureCoordinateIDMap overlayTextureCoordinateIDToUVIndex{};
  std::unordered_map<uint32_t, uint32_t> textureCoordinateMap;
};

// TODO: which ones of these explicit constructors are actually needed??
struct LoadMeshResult {
  LoadMeshResult() = default;
  LoadMeshResult(const LoadMeshResult& result) = delete;
  LoadMeshResult(LoadMeshResult&& result) = default;

  std::vector<LoadPrimitiveResult> primitiveResults;
};

struct LoadNodeResult {
  LoadNodeResult() = default;
  LoadNodeResult(const LoadNodeResult& result) = delete;
  LoadNodeResult(LoadNodeResult&& result) = default;

  std::optional<LoadMeshResult> meshResult = std::nullopt;
  std::unordered_map<int32_t, LoadMeshResult> meshVariantsResults;
  const CesiumGltf::ExtensionNodeMaxarMeshVariants* pVariantsExtension =
      nullptr;
};

struct LoadModelResult {
  LoadModelResult() = default;
  LoadModelResult(const LoadModelResult& result) = delete;

  std::vector<LoadNodeResult> nodeResults;
  FCesiumMetadataModel Metadata{};
  CesiumEncodedMetadataUtility::EncodedMetadata EncodedMetadata{};
  const CesiumGltf::ExtensionModelMaxarMeshVariants* pVariantsExtension =
      nullptr;
};
} // namespace LoadGltfResult
