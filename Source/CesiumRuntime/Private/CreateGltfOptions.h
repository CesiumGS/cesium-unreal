// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumEncodedMetadataComponent.h"
#include "CesiumFeaturesMetadataDescription.h"
#include "CesiumGltf/Mesh.h"
#include "CesiumGltf/MeshPrimitive.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltf/Node.h"
#include "LoadGltfResult.h"
#include "VoxelGridShape.h"

#include <Cesium3DTiles/ExtensionContent3dTilesContentVoxels.h>
#include <Cesium3DTilesSelection/TileLoadResult.h>

/**
 * Various settings and options for loading a glTF model from a 3D Tileset.
 */
namespace CreateGltfOptions {
struct CreateVoxelOptions;

struct CreateModelOptions {
  /**
   * A pointer to the glTF model.
   */
  CesiumGltf::Model* pModel = nullptr;

  /**
   * A description of which feature ID sets and metadata should be
   * encoded, taken from the tileset.
   */
  const FCesiumFeaturesMetadataDescription* pFeaturesMetadataDescription =
      nullptr;

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  const FMetadataDescription* pEncodedMetadataDescription_DEPRECATED = nullptr;
  PRAGMA_ENABLE_DEPRECATION_WARNINGS

  /**
   * Whether to always include tangents with the model. If the model includes
   * tangents and this setting is true, then the model's tangents will be used.
   * Otherwise, tangents will be generated for the model as it loads.
   */
  bool alwaysIncludeTangents = false;

  /**
   * Whether to create physics meshes for the model.
   */
  bool createPhysicsMeshes = true;

  /**
   * Whether to ignore the KHR_materials_unlit extension in the model. If this
   * is true and the extension is present, then flat normals will be generated
   * for the model as it loads.
   */
  bool ignoreKhrMaterialsUnlit = false;

  /**
   * Options for loading voxel primitives in the tileset, if present.
   */
  const CreateVoxelOptions* pVoxelOptions = nullptr;

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
        pVoxelOptions(other.pVoxelOptions),
        tileLoadResult(std::move(other.tileLoadResult)) {
    pModel = std::get_if<CesiumGltf::Model>(&this->tileLoadResult.contentKind);
  }

  CreateModelOptions(const CreateModelOptions&) = delete;
  CreateModelOptions& operator=(const CreateModelOptions&) = delete;
  CreateModelOptions& operator=(CreateModelOptions&&) = delete;
};

struct CreateNodeOptions {
  const CreateModelOptions* pModelOptions = nullptr;
  const LoadGltfResult::LoadedModelResult* pHalfConstructedModelResult =
      nullptr;
  const CesiumGltf::Node* pNode = nullptr;
};

struct CreateMeshOptions {
  const CreateNodeOptions* pNodeOptions = nullptr;
  const LoadGltfResult::LoadedNodeResult* pHalfConstructedNodeResult = nullptr;
  int32_t meshIndex = -1;
};

struct CreatePrimitiveOptions {
  const CreateMeshOptions* pMeshOptions = nullptr;
  const LoadGltfResult::LoadedMeshResult* pHalfConstructedMeshResult = nullptr;
  int32_t primitiveIndex = -1;
};

/**
 * Various settings and options for loading glTF voxels from a 3D Tileset.
 *
 * Currently these are used to validate voxels before construction, not so much
 * for configuring their creation.
 */
struct CreateVoxelOptions {
  /**
   *  The 3DTILES_content_voxels extension found on the tileset's root content.
   */
  const Cesium3DTiles::ExtensionContent3dTilesContentVoxels* pTilesetExtension;

  /**
   * A pointer to the class used by the tileset to define the voxel metadata.
   */
  const Cesium3DTiles::Class* pVoxelClass;

  /**
   * The shape of the voxel grid.
   */
  EVoxelGridShape gridShape;

  /**
   * The total number of voxels in the voxel grid, including padding. Used to
   * validate glTF voxel primitives for their amounts of attribute data.
   */
  size_t voxelCount;
};

} // namespace CreateGltfOptions
