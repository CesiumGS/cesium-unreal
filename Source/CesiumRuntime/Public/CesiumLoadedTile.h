// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "Cesium3DTilesSelection/Tile.h"
#include "CesiumModelMetadata.h"
#include "CesiumPrimitiveFeatures.h"
#include "CesiumPrimitiveMetadata.h"

#include "Math/Vector.h"
#include "UObject/ObjectMacros.h"

#include "CesiumLoadedTile.generated.h"

class ACesium3DTileset;
class UStaticMeshComponent;

UINTERFACE()
class UCesiumLoadedTile : public UInterface {
  GENERATED_BODY()
};

/**
 * Provides access to the details of a tile loaded by the @ref Cesium3DTileset.
 * This interface is implemented by the `USceneComponent` subclass that
 * represents the tile.
 */
class ICesiumLoadedTile {
  GENERATED_BODY()
public:
  /**
   * Gets the tile identifier: this is informational only, as there is no
   * guarantee of uniqueness.
   */
  virtual const Cesium3DTilesSelection::TileID& GetTileID() const = 0;

  /**
   * Gets the glTF model from which the tile data was obtained.
   */
  virtual const CesiumGltf::Model* GetGltfModel() const = 0;

  /**
   * Gets the scaling factor that was applied (via component-wise
   * multiplication) to the vertices of this tile's glTF model to obtain the
   * values represented in the corresponding Unreal mesh components. See @ref
   * ICesiumLoadedTilePrimitive::GetMeshComponent}.
   */
  virtual FVector GetGltfToUnrealLocalVertexPositionScaleFactor() const = 0;

  /**
   * Gets the tileset Actor that the tile belongs to.
   */
  virtual ACesium3DTileset& GetTilesetActor() = 0;

  /**
   * Gets the blueprint-accessible wrapper for metadata contained in the tile's
   * glTF model.
   */
  virtual const FCesiumModelMetadata& GetModelMetadata() const = 0;
};

// Not merged with ICesiumPrimitive because this is Public whereas
// ICesiumPrimitive is Private
UINTERFACE()
class UCesiumLoadedTilePrimitive : public UInterface {
  GENERATED_BODY()
};

/**
 * Provides access to the details of a glTF `MeshPrimitive` loaded by the @ref
 * Cesium3DTileset. This interface is implemented by the `USceneComponent`
 * subclass that represents a single glTF primitive that is part of the tile's
 * glTF model.
 */
class ICesiumLoadedTilePrimitive {
  GENERATED_BODY()
public:
  /**
   * Gets the loaded tile that this primitive belongs to.
   */
  virtual ICesiumLoadedTile& GetLoadedTile() = 0;

  /**
   * Gets the Unreal static mesh component built to represent the glTF
   * primitive.
   */
  virtual UStaticMeshComponent& GetMeshComponent() = 0;

  /**
   * Gets the Blueprint-accessible wrapper for the glTF Primitive's mesh
   * features.
   */
  virtual const FCesiumPrimitiveFeatures& GetPrimitiveFeatures() const = 0;

  /**
   * Gets the Blueprint-accessible wrapper for the glTF Primitive's
   * `EXT_structural_metadata` extension.
   */
  virtual const FCesiumPrimitiveMetadata& GetPrimitiveMetadata() const = 0;

  /**
   * Gets the glTF primitive.
   */
  virtual const CesiumGltf::MeshPrimitive* GetMeshPrimitive() const = 0;

  /**
   * Maps an accessor index in the glTF primitive to its corresponding texture
   * coordinate index in the Unreal mesh. The -1 key is reserved for implicit
   * feature IDs (in other words, the vertex index).
   *
   * @param AccessorIndex An accessor index in the glTF primitive
   * @return A texture coordinate index in the Unreal mesh, or std::nullopt if
   * none was found for the AccessorIndex passed.
   */
  virtual std::optional<uint32_t>
  FindTextureCoordinateIndexForGltfAccessor(int32_t AccessorIndex) const = 0;
};
