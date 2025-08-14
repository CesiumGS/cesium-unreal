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
class ICesiumLoadedTile {
  GENERATED_BODY()
public:
  /** Get the tile identifier: this is informational only, as there is no
   * guarantee of uniqueness */
  virtual const Cesium3DTilesSelection::TileID& GetTileID() const = 0;
  virtual const CesiumGltf::Model* GetGltfModel() const = 0;
  /** Scaling factor to be applied (component-wise multiplication) to glTF
   * vertices of this tile's model to obtain the values represented in their
   * matching mesh component (see @{link
   * UCesiumLoadedTilePrimitive::GetMeshComponent). */
  virtual FVector GetGltfToUnrealLocalVertexPositionScaleFactor() const = 0;
  virtual ACesium3DTileset& GetTilesetActor() = 0;
  virtual const FCesiumModelMetadata& GetModelMetadata() const = 0;
};

UINTERFACE()
class UCesiumLoadedTilePrimitive : public UInterface {
  GENERATED_BODY()
};
// Not merged with ICesiumPrimitive because this is Public whereas
// ICesiumPrimitive is Private
class ICesiumLoadedTilePrimitive {
  GENERATED_BODY()
public:
  virtual ICesiumLoadedTile& GetLoadedTile() = 0;
  virtual UStaticMeshComponent& GetMeshComponent() = 0;
  virtual const FCesiumPrimitiveFeatures& GetPrimitiveFeatures() const = 0;
  virtual const FCesiumPrimitiveMetadata& GetPrimitiveMetadata() const = 0;
  virtual const CesiumGltf::MeshPrimitive* GetMeshPrimitive() const = 0;
  virtual std::optional<uint32_t>
  FindTexCoordIndexForGltfAttribute(int32_t accessorIndex) const = 0;
};
