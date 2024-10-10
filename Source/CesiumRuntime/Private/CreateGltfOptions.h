// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumEncodedMetadataComponent.h"
#include "CesiumGltf/Mesh.h"
#include "CesiumGltf/MeshPrimitive.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltf/Node.h"
#include "LoadGltfResult.h"

// TODO: internal documentation
namespace CreateGltfOptions {
struct CreateModelOptions {
  const FCesiumFeaturesMetadataDescription* pFeaturesMetadataDescription =
      nullptr;
  CesiumGltf::Model* pModel = nullptr;

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  const FMetadataDescription* pEncodedMetadataDescription_DEPRECATED = nullptr;
  PRAGMA_ENABLE_DEPRECATION_WARNINGS
  bool alwaysIncludeTangents = false;
  bool createPhysicsMeshes = true;
  bool ignoreKhrMaterialsUnlit = false;

  Cesium3DTilesSelection::TileLoadResult tileLoadResult;

public:
  CreateModelOptions(Cesium3DTilesSelection::TileLoadResult&& tileLoadResult_)
      : tileLoadResult(std::move(tileLoadResult_)) {
    pModel = std::get_if<CesiumGltf::Model>(&this->tileLoadResult.contentKind);
  }

  CreateModelOptions(CreateModelOptions&& other)
      : pFeaturesMetadataDescription(other.pFeaturesMetadataDescription),
        pEncodedMetadataDescription_DEPRECATED(
            other.pEncodedMetadataDescription_DEPRECATED),
        alwaysIncludeTangents(other.alwaysIncludeTangents),
        createPhysicsMeshes(other.createPhysicsMeshes),
        ignoreKhrMaterialsUnlit(other.ignoreKhrMaterialsUnlit),
        tileLoadResult(std::move(other.tileLoadResult)) {
    pModel = std::get_if<CesiumGltf::Model>(&this->tileLoadResult.contentKind);
  }

  CreateModelOptions(const CreateModelOptions&) = delete;
  CreateModelOptions& operator=(const CreateModelOptions&) = delete;
  CreateModelOptions& operator=(CreateModelOptions&&) = delete;
};

struct CreateNodeOptions {
  const CreateModelOptions* pModelOptions = nullptr;
  const LoadGltfResult::LoadModelResult* pHalfConstructedModelResult = nullptr;
  const CesiumGltf::Node* pNode = nullptr;
};

struct CreateMeshOptions {
  const CreateNodeOptions* pNodeOptions = nullptr;
  const LoadGltfResult::LoadNodeResult* pHalfConstructedNodeResult = nullptr;
  int32_t meshIndex = -1;
};

struct CreatePrimitiveOptions {
  const CreateMeshOptions* pMeshOptions = nullptr;
  const LoadGltfResult::LoadMeshResult* pHalfConstructedMeshResult = nullptr;
  int32_t primitiveIndex = -1;
};
} // namespace CreateGltfOptions
