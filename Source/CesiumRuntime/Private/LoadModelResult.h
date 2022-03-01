// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

struct LoadPrimitiveResult {
  FCesiumMetadataPrimitive Metadata{};
  CesiumTextureUtility::EncodedMetadataPrimitive EncodedMetadata;
  FStaticMeshRenderData* RenderData = nullptr;
  const CesiumGltf::Model* pModel = nullptr;
  const CesiumGltf::MeshPrimitive* pMeshPrimitive = nullptr;
  const CesiumGltf::Material* pMaterial = nullptr;
  glm::dmat4x4 transform{1.0};
#if PHYSICS_INTERFACE_PHYSX
  PxTriangleMesh* pCollisionMesh = nullptr;
  FBodySetupUVInfo uvInfo;
#else
  TSharedPtr<Chaos::FTriangleMeshImplicitObject, ESPMode::ThreadSafe>
      pCollisionMesh = nullptr;
#endif
  std::string name{};

  CesiumTextureUtility::LoadedTextureResult* baseColorTexture = nullptr;
  CesiumTextureUtility::LoadedTextureResult* metallicRoughnessTexture = nullptr;
  CesiumTextureUtility::LoadedTextureResult* normalTexture = nullptr;
  CesiumTextureUtility::LoadedTextureResult* emissiveTexture = nullptr;
  CesiumTextureUtility::LoadedTextureResult* occlusionTexture = nullptr;
  CesiumTextureUtility::LoadedTextureResult* waterMaskTexture = nullptr;
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