// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

struct LoadPrimitiveResult {
  FCesiumMetadataPrimitive Metadata{};
  TUniquePtr<FStaticMeshRenderData> RenderData = nullptr;
  const CesiumGltf::Model* pModel = nullptr;
  const CesiumGltf::MeshPrimitive* pMeshPrimitive = nullptr;
  const CesiumGltf::Material* pMaterial = nullptr;
  glm::dmat4x4 transform{1.0};
#if PHYSICS_INTERFACE_PHYSX
  PxTriangleMesh* pCollisionMesh = nullptr;
#else
  TSharedPtr<Chaos::FTriangleMeshImplicitObject, ESPMode::ThreadSafe>
      pCollisionMesh = nullptr;
#endif
  std::string name{};

  TUniquePtr<CesiumTextureUtility::LoadedTextureResult> baseColorTexture = nullptr;
  TUniquePtr<CesiumTextureUtility::LoadedTextureResult> metallicRoughnessTexture = nullptr;
  TUniquePtr<CesiumTextureUtility::LoadedTextureResult> normalTexture = nullptr;
  TUniquePtr<CesiumTextureUtility::LoadedTextureResult> emissiveTexture = nullptr;
  TUniquePtr<CesiumTextureUtility::LoadedTextureResult> occlusionTexture = nullptr;
  TUniquePtr<CesiumTextureUtility::LoadedTextureResult> waterMaskTexture = nullptr;
  std::unordered_map<std::string, uint32_t> textureCoordinateParameters;

  bool onlyLand = true;
  bool onlyWater = false;

  double waterMaskTranslationX = 0.0;
  double waterMaskTranslationY = 0.0;
  double waterMaskScale = 1.0;

  OverlayTextureCoordinateIDMap overlayTextureCoordinateIDToUVIndex{};
};

struct LoadMeshResult {
  std::vector<LoadPrimitiveResult> primitiveResults{};
};

struct LoadNodeResult {
  std::optional<LoadMeshResult> meshResult = std::nullopt;
};

struct LoadModelResult {
  std::vector<LoadNodeResult> nodeResults{};
};
