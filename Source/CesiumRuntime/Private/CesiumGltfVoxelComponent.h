// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumPrimitiveMetadata.h"
#include "CoreMinimal.h"

#include <CesiumGeometry/OctreeTileID.h>
#include <unordered_map>

#include "CesiumGltfVoxelComponent.generated.h"

/**
 * A barebones component representing a glTF voxel primitive.
 *
 * This does not hold any mesh data itself; instead, it contains the property
 * attribute used.UCesiumVoxelRendererComponent takes care of voxel rendering
 * for an entire tileset.
 */
UCLASS()
class UCesiumGltfVoxelComponent : public USceneComponent {
  GENERATED_BODY()

public:
  // Sets default values for this component's properties
  UCesiumGltfVoxelComponent();
  virtual ~UCesiumGltfVoxelComponent();

  void BeginDestroy();

  CesiumGeometry::OctreeTileID TileId;
  FCesiumPropertyAttribute PropertyAttribute;
};
