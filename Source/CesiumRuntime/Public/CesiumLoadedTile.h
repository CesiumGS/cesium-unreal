// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "Cesium3DTilesSelection/Tile.h"
#include "CesiumModelMetadata.h"
#include "CesiumPrimitiveFeatures.h"
#include "CesiumPrimitiveMetadata.h"

#include "UObject/ObjectMacros.h"

#include "CesiumLoadedTile.generated.h"

class UStaticMeshComponent;

UINTERFACE()
class UCesiumLoadedTileBase : public UInterface {
  GENERATED_BODY()
};
class ICesiumLoadedTileBase {
  GENERATED_BODY()
public:
  virtual Cesium3DTilesSelection::TileID const& GetTileID() const = 0;
  virtual FCesiumModelMetadata const& GetModelMetadata() const = 0;
};

UINTERFACE()
class UCesiumLoadedTile : public UCesiumLoadedTileBase {
  GENERATED_BODY()
};
class ICesiumLoadedTile : public ICesiumLoadedTileBase {
  GENERATED_BODY()
public:
  virtual void SetRenderReady(bool bToggle) = 0;
};

UINTERFACE()
class UCesiumLoadedTilePrimitive : public UCesiumLoadedTileBase {
  GENERATED_BODY()
};
// Not merged with ICesiumPrimitive because this is Public whereas ICesiumPrimitive is Private
class ICesiumLoadedTilePrimitive : public ICesiumLoadedTileBase {
  GENERATED_BODY()
public:
  virtual UStaticMeshComponent& GetMeshComponent() = 0;
  virtual FCesiumPrimitiveFeatures const& GetPrimitiveFeatures() const = 0;
  virtual FCesiumPrimitiveMetadata const& GetPrimitiveMetadata() const = 0;
  virtual const CesiumGltf::MeshPrimitive* GetMeshPrimitive() const = 0;
  virtual std::optional<uint32_t> FindTexCoordIndexForGltfAttribute(int32_t accessorIndex) const = 0;
};
