// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumPrimitiveMetadata.h"
#include "CoreMinimal.h"

#include <CesiumGeometry/OctreeTileID.h>
#include <unordered_map>

#include "CesiumGltfVoxelComponent.generated.h"

/**
 * A minimal component representing a glTF voxel primitive.
 *
 * This component is not a mesh component. Instead, it contains the property
 * attribute used for the voxel primitive. It is \ref
 * UCesiumVoxelRendererComponent that handles voxel rendering for the entire
 * tileset.
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
