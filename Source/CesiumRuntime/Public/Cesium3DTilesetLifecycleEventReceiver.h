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
namespace CesiumGltf {
struct Material;
struct MaterialPBRMetallicRoughness;
} // namespace CesiumGltf

UINTERFACE()
class UCesium3DTilesetLifecycleEventReceiver : public UInterface {
  GENERATED_BODY()
};
/** Extension points for customizations requiring mesh and tile properties and
 * lifecycle information. All methods are called from the game thread.*/
class CESIUMRUNTIME_API ICesium3DTilesetLifecycleEventReceiver {
  GENERATED_BODY()
public:
  /**
   * Allows to override the base material from which the given tile primitive's
   *material instance will be created. The default implementation of this method
   *can be used to create the material when no customization is needed.
   * @param TilePrim Loaded tile primitive for which a material is needed
   * @param pDefaultBaseMaterial Default chosen base material. May be ignored if
   *the method chooses to create the mesh material based on a custom base
   *material.
   * @param Outer The outer for the new material, as used by creation functions
   *like NewObject
   * @param Name The name for the new material, as used by creation functions
   *like NewObject
   * @return Material instance created. If null, a material will be created by
   *the caller based on the pBaseMaterial passed, ie. as if this method had not
   *been called.
   */
  virtual UMaterialInstanceDynamic* CreateMaterial(
      ICesiumLoadedTilePrimitive& TilePrim,
      UMaterialInterface* pDefaultBaseMaterial,
      UObject* Outer,
      const FName& Name) = 0;

  /**
   * Customize the Unreal material instance, depending on the glTF material
   * definition.
   * @param TilePrim Loaded tile primitive to which the material applies
   * @param glTFmaterial Parameters of the glTF material for the primitive
   * @param glTFmaterial Parameters for this primitive's material defining the
   * metallic-roughness material model from Physically-Based Rendering (PBR)
   * methodology
   * @param pMaterial Unreal material created for the primitive
   * @param association Type of association (layer, blend, global) being
   * configured
   * @param index Index of the layer or blend being configured (see
   * association). Not relevant for global association.
   */
  virtual void CustomizeGltfMaterial(
      ICesiumLoadedTilePrimitive& TilePrim,
      const CesiumGltf::Material& glTFmaterial,
      const CesiumGltf::MaterialPBRMetallicRoughness& pbr,
      UMaterialInstanceDynamic* pMaterial,
      EMaterialParameterAssociation association,
      int32 index);

  /**
   * Called at the end of the static mesh component construction.
   * @param TilePrim Loaded tile primitive being constructed
   */
  virtual void
  OnTileMeshPrimitiveConstructed(ICesiumLoadedTilePrimitive& TilePrim) = 0;

  /**
   * Called at the end of all static mesh components' construction for a given
   * tile.
   * @param LoadedTile The tile that has just been loaded
   */
  virtual void OnTileConstructed(ICesiumLoadedTile& LoadedTile) = 0;

  /**
   * Called when changing the visibility of any UCesiumGltfComponent, ie usually
   * several times per tile (when the tileset selection leads to showing or
   * hiding a whole tile).
   * @param LoadedTile The tile which visibility is being toggled
   * @param visible New visibility flag being applied
   */
  virtual void
  OnVisibilityChanged(ICesiumLoadedTile& LoadedTile, bool visible) = 0;

  /**
   * Called before a tile is destroyed (when it is unloaded, typically).
   * @param LoadedTile The tile which is about to be unloaded
   */
  virtual void BeforeTileDestruction(ICesiumLoadedTile& LoadedTile) = 0;
};
