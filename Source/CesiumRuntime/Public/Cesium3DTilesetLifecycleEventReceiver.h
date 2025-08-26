// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#pragma once

#include "Cesium3DTilesSelection/TileID.h"
#include "CesiumLoadedTile.h"
#include "UObject/ObjectMacros.h"

#include "Cesium3DTilesetLifecycleEventReceiver.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
enum EMaterialParameterAssociation : int;

class ICesiumLoadedTile;
class ICesiumLoadedTilePrimitive;
class UCesiumMaterialUserData;
namespace CesiumGltf {
struct Material;
struct MaterialPBRMetallicRoughness;
} // namespace CesiumGltf

// Note: to allow implementation in Blueprints:
// 1. remove this 'meta' flag, and make the interface methods below
// compatible with a Blueprint implementation,
// 2. remove the Cast<ICesium3DTilesetLifecycleEventReceiver> in
// ACesium3DTileset::GetLifecycleEventReceiver, as it would return nullptr for a
// BP implementation, and return the UObject pointer instead,
// 3. use ICesium3DTilesetLifecycleEventReceiver::Execute_DoStuff wrappers
// instead of DoStuff methods everywhere the event receiver is used in the
// plugin's C++ code.
UINTERFACE(meta = (CannotImplementInterfaceInBlueprint))
class UCesium3DTilesetLifecycleEventReceiver : public UInterface {
  GENERATED_BODY()
};
/** Extension points for customizations requiring mesh and tile properties and
 * lifecycle information. All methods are called from the game thread.*/
class CESIUMRUNTIME_API ICesium3DTilesetLifecycleEventReceiver {
  GENERATED_BODY()
public:
  /**
   * Creates a material instance for a given tile primitive.
   * The default implementation simply calls `UMaterialInstanceDynamic::Create`
   * with the default base material. Overriding this method is useful when a
   * different base material should be selected based on properties of the
   * primitive.
   * @param TilePrimitive Loaded tile primitive for which a material is needed
   * @param DefaultBaseMaterial Default chosen base material. May be ignored if
   * the method chooses to create the mesh material based on a custom base
   * material.
   * @param Name The name for the new material, as used by creation functions
   * like NewObject
   * @return Material instance created: should not be nullptr.
   */
  virtual UMaterialInstanceDynamic* CreateMaterial(
      ICesiumLoadedTilePrimitive& TilePrimitive,
      UMaterialInterface* DefaultBaseMaterial,
      const FName& Name);

  /**
   * Customize the Unreal material instance, depending on the glTF material
   * definition.
   * @param TilePrimitive Loaded tile primitive to which the material applies
   * @param Material Unreal material created for the primitive
   * @param CesiumData List of material layer names
   * @param GlTFmaterial Parameters of the glTF material for the primitive
   */
  virtual void CustomizeMaterial(
      ICesiumLoadedTilePrimitive& TilePrimitive,
      UMaterialInstanceDynamic& Material,
      const UCesiumMaterialUserData* CesiumData,
      const CesiumGltf::Material& GlTFmaterial);

  /**
   * Called after a `MeshPrimitive` in a tile's glTF is loaded. This method is
   * called at the end of the load process, after construction of the static
   * mesh component that will render the primitive.
   *
   * @param TilePrimitive Tile primitive that has just been loaded.
   */
  virtual void
  OnTileMeshPrimitiveLoaded(ICesiumLoadedTilePrimitive& TilePrimitive);

  /**
   * Called after a new tile has been loaded. This method is called after
   * `OnTileMeshPrimitiveLoaded` has been called for all of the tile's
   * primitives.
   *
   * @param Tile The tile that has just been loaded
   */
  virtual void OnTileLoaded(ICesiumLoadedTile& Tile);

  /**
   * Called when a tile is shown or hidden. This may be called zero or more
   * times per tile.
   * @param Tile The tile for which visibility is being toggled
   * @param bVisible New visibility flag being applied
   */
  virtual void
  OnTileVisibilityChanged(ICesiumLoadedTile& Tile, bool bVisible);

  /**
   * Called before a tile is unloaded.
   * @param Tile The tile that is about to be unloaded
   */
  virtual void OnTileUnloading(ICesiumLoadedTile& Tile);
};
