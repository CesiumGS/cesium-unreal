//// Copyright 2020-2024 CesiumGS, Inc. and Contributors
//
//#pragma once
//
//#include "CesiumCommon.h"
//#include "CesiumGltfVoxelComponent.h"
//#include "Engine/VolumeTexture.h"
//#include "VoxelDataTextures.h"
//#include "VoxelOctree.h"
//
//#include <Cesium3DTiles/Class.h>
//#include <Cesium3DTilesSelection/Tile.h>
//#include <CesiumGeometry/OctreeTileID.h>
//#include <CesiumGltf/Model.h>
//
//#include <glm/glm.hpp>
//#include <memory>
//#include <queue>
//#include <set>
//#include <unordered_map>
//
//namespace Cesium3DTilesSelection {
//class Tile;
//}
//
//enum class EVoxelGridShape : uint8;
//struct FCesiumVoxelClassDescription;
//
//class FVoxelResources {
//public:
//  /**
//   * Value constants taken from CesiumJS.
//   */
//  static const uint32 MaximumOctreeTextureWidth = 2048;
//  static const uint32 MaximumDataTextureMemoryBytes = 512 * 1024 * 1024;
//  static const uint32 DefaultDataTextureMemoryBytes = 128 * 1024 * 1024;
//
//  /**
//   * @brief Constructs the resources necessary to render voxel data in an Unreal
//   * material.
//   *
//   * @param pVoxelClass The voxel class description, indicating which metadata
//   * attributes to encode.
//   * @param shape The shape of the voxel grid, which affects how voxel data is
//   * read and stored.
//   * @param dataDimensions The dimensions of the voxel data in each tile,
//   * including padding.
//   * @param featureLevel The RHI feature level associated with the scene.
//   * @param requestedMemoryPerDataTexture The requested texture memory for the
//   * data texture constructed for each voxel attribute.
//   */
//  FVoxelResources(
//      const FCesiumVoxelClassDescription* pVoxelClass,
//      EVoxelGridShape shape,
//      const glm::uvec3& dataDimensions,
//      ERHIFeatureLevel::Type featureLevel,
//      uint32 requestedMemoryPerDataTexture = DefaultDataTextureMemoryBytes);
//  ~FVoxelResources();
//
//  /**
//   * @brief Retrieves how many tiles there in the megatexture along each
//   * dimension.
//   */
//  FVector GetTileCount() const;
//
//  /**
//   * @brief Retrieves the texture containing the encoded octree.
//   */
//  UTexture2D* GetOctreeTexture() const { return this->_octree.getTexture(); }
//
//  /**
//   * @brief Retrieves the texture containing the data for the attribute with
//   * the given ID. Returns nullptr if the attribute does not exist.
//   */
//  UTexture* GetDataTexture(const FString& attributeId) const {
//    return this->_dataTextures.getTexture(attributeId);
//  }
//
//  /**
//   * Updates the resources given the currently visible tiles.
//   */
//  void Update(
//      const std::vector<Cesium3DTilesSelection::Tile::Pointer>& VisibleTiles,
//      const std::vector<double>& VisibleTileScreenSpaceErrors);
//
//private:
//  static double computePriority(double sse);
//
//  struct VoxelTileUpdateInfo {
//    const UCesiumGltfVoxelComponent* pComponent;
//    double sse;
//    double priority;
//  };
//
//  struct ScreenSpaceErrorGreaterComparator {
//    bool
//    operator()(const VoxelTileUpdateInfo& lhs, const VoxelTileUpdateInfo& rhs) {
//      return lhs.priority > rhs.priority;
//    }
//  };
//
//  struct ScreenSpaceErrorLessComparator {
//    bool
//    operator()(const VoxelTileUpdateInfo& lhs, const VoxelTileUpdateInfo& rhs) {
//      return lhs.priority < rhs.priority;
//    }
//  };
//
//  FVoxelOctree _octree;
//  UVoxelDataTextures _dataTextures;
//
//  using MaxPriorityQueue = std::priority_queue<
//      VoxelTileUpdateInfo,
//      std::vector<VoxelTileUpdateInfo>,
//      ScreenSpaceErrorLessComparator>;
//
//  std::vector<CesiumGeometry::OctreeTileID> _loadedNodeIds;
//  MaxPriorityQueue _visibleTileQueue;
//};
