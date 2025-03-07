// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include <CesiumGeometry/OctreeTileID.h>
#include <unordered_map>

#include "CesiumGltfVoxelComponent.generated.h"

namespace CesiumGltf {
struct Model;
struct MeshPrimitive;
struct PropertyAttribute;
struct Accessor;
struct BufferView;
struct Buffer;
} // namespace CesiumGltf


class ACesium3DTileset;

/**
 * Pointers to a buffer that has already been validated, such that:
 * - The accessor count is equal to the number of total voxels in the grid.
 * - The buffer view on the buffer is valid.
 *
 * This should be replaced when PropertyAttributeProperty is supported, since it
 * is functionally the same (and the latter would be more robust).
 */
struct ValidatedVoxelBuffer {
  const CesiumGltf::Buffer* pBuffer;
  const CesiumGltf::BufferView* pBufferView;
};

/**
 * A barebones component representing a glTF voxel primitive.
 *
 * The voxel rendering for an entire tileset is done singlehandedly by
 * UCesiumVoxelRendererComponent. Therefore, this component does not hold any
 * mesh data itself. Instead, it stores pointers to the glTF primitive for easy
 * retrieval of the voxel attributes.
 */
UCLASS()
class UCesiumGltfVoxelComponent : public USceneComponent {
  GENERATED_BODY()

public:
  // Sets default values for this component's properties
  UCesiumGltfVoxelComponent();
  virtual ~UCesiumGltfVoxelComponent();

  void BeginDestroy();

  CesiumGeometry::OctreeTileID tileId;
  TMap<FString, ValidatedVoxelBuffer> attributeBuffers;
};
