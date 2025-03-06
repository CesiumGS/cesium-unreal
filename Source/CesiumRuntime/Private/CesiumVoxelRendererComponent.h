// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "VoxelResources.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CoreMinimal.h"
#include "CreateGltfOptions.h"
#include "CustomDepthParameters.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Templates/UniquePtr.h"
#include "VoxelGridShape.h"

#include <glm/glm.hpp>
#include <optional>

#include "CesiumVoxelRendererComponent.generated.h"

namespace Cesium3DTilesSelection {
class Tile;
class TilesetMetadata;
} // namespace Cesium3DTilesSelection

class ACesium3DTileset;
struct FCesiumVoxelClassDescription;

UCLASS()
/**
 * A component that enables raycasted voxel rendering. This is only attached to
 * a Cesium3DTileset when it contains voxel data.
 *
 * Unlike typical glTF meshes, voxels are rendered by raycasting in an Unreal
 * material attached to a placeholder cube mesh.
 */
class UCesiumVoxelRendererComponent : public USceneComponent {
  GENERATED_BODY()

public:
  static UCesiumVoxelRendererComponent* Create(
      ACesium3DTileset* pTilesetActor,
      const Cesium3DTilesSelection::TilesetMetadata& tilesetMetadata,
      const Cesium3DTilesSelection::Tile& rootTile,
      const Cesium3DTiles::ExtensionContent3dTilesContentVoxels& voxelExtension,
      const FCesiumVoxelClassDescription* pDescription);

  // Sets default values for this component's properties
  UCesiumVoxelRendererComponent();
  virtual ~UCesiumVoxelRendererComponent();

  void BeginDestroy() override;
  UPROPERTY(EditAnywhere, Category = "Cesium")
  UMaterialInterface* DefaultMaterial = nullptr;

  UPROPERTY(EditAnywhere, Category = "Cesium")
  UStaticMesh* CubeMesh = nullptr;

  /**
   * The mesh used to render the voxels.
   */
  UStaticMeshComponent* MeshComponent = nullptr;

  /**
   * The options for creating voxel primitives based on the tileset's
   * 3DTILES_content_voxels extension. This is referenced during the glTF load
   * process.
   */
  CreateGltfOptions::CreateVoxelOptions Options;

  /**
   * The double-precision transformation matrix for the root tile of the
   * tileset.
   */
  glm::dmat4x4 HighPrecisionTransform;

  /**
   * Updates the voxel renderer based on the newly visible tiles.
   *
   * @param visibleTiles The visible tiles.
   * @param visibleTileScreenSpaceErrors The screen space error values computed
   * this frame for the visible tiles. Used to compute priority for voxel tile
   * rendering.
   */
  void UpdateTiles(
      const std::vector<Cesium3DTilesSelection::Tile*>& VisibleTiles,
      const std::vector<double>& VisibleTileScreenSpaceErrors);

  void UpdateTransformFromCesium(const glm::dmat4& CesiumToUnrealTransform);

private:
  static UMaterialInstanceDynamic* CreateVoxelMaterial(
      UCesiumVoxelRendererComponent* pVoxelComponent,
      const FVector& dimensions,
      const FVector& paddingBefore,
      const FVector& paddingAfter,
      ACesium3DTileset* pTilesetActor,
      const Cesium3DTiles::Class* pVoxelClass,
      const FCesiumVoxelClassDescription* pDescription,
      const Cesium3DTilesSelection::BoundingVolume& boundingVolume);

  /**
   * The resources used to render voxels across the tileset.
   */
  TUniquePtr<FVoxelResources> _pResources = nullptr;

  /**
   * The tileset that owns this voxel renderer.
   */
  ACesium3DTileset* _pTileset = nullptr;
};
