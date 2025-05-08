/*--------------------------------------------------------------------------------------+
|
|     $Source: CesiumMeshBuildCallbacks.h $
|
|  $Copyright: (c) 2024 Bentley Systems, Incorporated. All rights reserved. $
|
+--------------------------------------------------------------------------------------*/

#pragma once

#include <Cesium3DTilesSelection/TileID.h>
#include <CesiumLoadedTile.h>
#include <Components.h>
#include <UObject/WeakObjectPtr.h>

#include <optional>
#include <unordered_map>

class UMaterialInstanceDynamic;
class UMaterialInterface;
enum EMaterialParameterAssociation : int;
class UStaticMeshComponent;
class USceneComponent;
struct FCesiumModelMetadata;
struct FCesiumPrimitiveFeatures;
class ICesiumTilePrimitiveData;

namespace CesiumGltf {
struct MeshPrimitive;
struct Material;
struct MaterialPBRMetallicRoughness;
} // namespace CesiumGltf
namespace Cesium3DTilesSelection {
class Tile;
}

/** Extension points for customizations requiring mesh and tile properties and
 * lifecycle information. All methods are called from the game thread.*/
class CESIUMRUNTIME_API CesiumMeshBuildCallbacks {
public:
  CesiumMeshBuildCallbacks();
  virtual ~CesiumMeshBuildCallbacks();

  /**
   * Allows to override the base material from which loadPrimitiveGameThreadPart
   *creates a dynamic material instance for the given primitive. The method can
   *optionally create the material itself, if further customizations need to be
   *done before returning the material, or can let the caller function do it
   *based on InOut_pChosenBaseMaterial.
   * \param InOut_pChosenBaseMaterial Input passes the default chosen base
   *material. Can be modified so that the caller will create the material
   *instance based on the custom base material (unless this method creates the
   *material itself).
   * \return Material instance created, or nullptr to let the caller create it.
   */
  virtual UMaterialInstanceDynamic* CreateMaterial(
      ICesiumLoadedTilePrimitive& TilePrim,
      UMaterialInterface*& InOut_pChosenBaseMaterial,
      UObject* InOuter,
      FName const& Name) = 0;

  /**
   * Customize the Unreal material instance, depending on the glTF material
   * definition.
   */
  virtual void CustomizeGltfMaterial(
      const CesiumGltf::Material& glTFmaterial,
      const CesiumGltf::MaterialPBRMetallicRoughness& pbr,
      UMaterialInstanceDynamic* pMaterial,
      EMaterialParameterAssociation association,
      int32 index) const;

  /**
   * Called at the end of the static mesh component construction.
   */
  virtual void OnMeshConstructed(
      ICesiumLoadedTile& LoadedTile,
      ICesiumLoadedTilePrimitive& TilePrim) = 0;

  /**
   * Called at the end of all static mesh components' construction for a given
   * tile.
   */
  virtual void
  OnTileConstructed(const Cesium3DTilesSelection::TileID& TileID) = 0;

  /**
   * Called when changing the visibility of any UCesiumGltfComponent, ie usually
   * several times per tile (when the tileset selection leads to showing or
   * hiding a whole tile).
   */
  virtual void OnVisibilityChanged(
      const Cesium3DTilesSelection::TileID& TileID,
      bool visible) = 0;

  /**
   * Called before a tile is destroyed (when it is unloaded, typically).
   */
  virtual void
  BeforeTileDestruction(const Cesium3DTilesSelection::TileID& TileID) = 0;

private:
  static TSharedPtr<CesiumMeshBuildCallbacks> Singleton;
};
