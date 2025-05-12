// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "VoxelResources.h"
#include "CesiumGltfComponent.h"
#include "CesiumRuntime.h"
#include "EncodedMetadataConversions.h"
#include "VoxelGridShape.h"

#include <Cesium3DTilesContent/ImplicitTilingUtilities.h>
#include <Cesium3DTilesSelection/Tile.h>
#include <CesiumGltf/ExtensionMeshPrimitiveExtStructuralMetadata.h>
#include <CesiumGltf/ExtensionModelExtStructuralMetadata.h>
#include <CesiumGltf/PropertyType.h>

#include <array>
#include <cmath>

using namespace CesiumGltf;
using namespace Cesium3DTilesContent;

FVoxelResources::FVoxelResources(
    const FCesiumVoxelClassDescription* pVoxelClass,
    EVoxelGridShape shape,
    const glm::uvec3& dataDimensions,
    ERHIFeatureLevel::Type featureLevel,
    uint32 requestedMemoryPerDataTexture)
    : _dataTextures(
          pVoxelClass,
          dataDimensions,
          featureLevel,
          requestedMemoryPerDataTexture),
      _loadedNodeIds(),
      _visibleTileQueue() {
  uint32 width = MaximumOctreeTextureWidth;
  uint32 maximumTileCount = this->_dataTextures.GetMaximumTileCount();
  this->_octree.InitializeTexture(width, maximumTileCount);
  this->_loadedNodeIds.reserve(maximumTileCount);
}

FVoxelResources::~FVoxelResources() {}

FVector FVoxelResources::GetTileCount() const {
  auto tileCount = this->_dataTextures.GetTileCountAlongAxes();
  return FVector(tileCount.x, tileCount.y, tileCount.z);
}

namespace {
template <typename Func>
void forEachRenderableVoxelTile(const auto& tiles, Func&& f) {
  for (size_t i = 0; i < tiles.size(); i++) {
    Cesium3DTilesSelection::Tile* pTile = tiles[i];
    if (!pTile ||
        pTile->getState() != Cesium3DTilesSelection::TileLoadState::Done) {
      continue;
    }

    const Cesium3DTilesSelection::TileContent& content = pTile->getContent();
    const Cesium3DTilesSelection::TileRenderContent* pRenderContent =
        content.getRenderContent();
    if (!pRenderContent) {
      continue;
    }

    UCesiumGltfComponent* Gltf = static_cast<UCesiumGltfComponent*>(
        pRenderContent->getRenderResources());
    if (!Gltf) {
      // When a tile does not have render resources (i.e. a glTF), then
      // the resources either have not yet been loaded or prepared,
      // or the tile is from an external tileset and does not directly
      // own renderable content. In both cases, the tile is ignored here.
      continue;
    }

    const TArray<USceneComponent*>& Children = Gltf->GetAttachChildren();
    for (USceneComponent* pChild : Children) {
      UCesiumGltfVoxelComponent* pVoxelComponent =
          Cast<UCesiumGltfVoxelComponent>(pChild);
      if (!pVoxelComponent || pVoxelComponent->attributeBuffers.IsEmpty()) {
        continue;
      }

      f(i, pTile, pVoxelComponent);
    }
  }
}
} // namespace

void FVoxelResources::Update(
    const std::vector<Cesium3DTilesSelection::Tile*>& VisibleTiles,
    const std::vector<double>& VisibleTileScreenSpaceErrors) {
  forEachRenderableVoxelTile(
      VisibleTiles,
      [&VisibleTileScreenSpaceErrors,
       &priorityQueue = this->_visibleTileQueue,
       &octree = this->_octree](
          size_t index,
          const Cesium3DTilesSelection::Tile* pTile,
          const UCesiumGltfVoxelComponent* pVoxel) {
        double sse = VisibleTileScreenSpaceErrors[index];
        FVoxelOctree::Node* pNode = octree.GetNode(pVoxel->tileId);
        if (pNode) {
          pNode->LastKnownScreenSpaceError = sse;
        }

        // Don't create the missing node just yet? It may not be added to the
        // tree depending on the priority of other nodes.
        priorityQueue.push({pVoxel, sse, computePriority(sse)});
      });

  if (this->_visibleTileQueue.empty()) {
    return;
  }

  // Sort the existing nodes in the megatexture by highest to lowest priority.
  std::sort(
      this->_loadedNodeIds.begin(),
      this->_loadedNodeIds.end(),
      [&octree = this->_octree](
          const CesiumGeometry::OctreeTileID& lhs,
          const CesiumGeometry::OctreeTileID& rhs) {
        FVoxelOctree::Node* pLeft = octree.GetNode(lhs);
        FVoxelOctree::Node* pRight = octree.GetNode(rhs);
        if (!pLeft) {
          return false;
        }
        if (!pRight) {
          return true;
        }
        return computePriority(pLeft->LastKnownScreenSpaceError) >
               computePriority(pRight->LastKnownScreenSpaceError);
      });

  bool shouldUpdateOctree = false;
  // It is possible for the data textures to not exist (e.g., the default voxel
  // material), so check this explicitly.
  bool dataTexturesExist = this->_dataTextures.GetTextureCount() > 0;

  size_t existingNodeCount = this->_loadedNodeIds.size();
  size_t destroyedNodeCount = 0;
  size_t addedNodeCount = 0;

  if (dataTexturesExist) {
    // For all of the visible nodes...
    for (; !this->_visibleTileQueue.empty(); this->_visibleTileQueue.pop()) {
      const VoxelTileUpdateInfo& currentTile = this->_visibleTileQueue.top();
      const CesiumGeometry::OctreeTileID& currentTileId =
          currentTile.pComponent->tileId;
      FVoxelOctree::Node* pNode = this->_octree.GetNode(currentTileId);
      if (pNode && pNode->DataSlotIndex >= 0) {
        // Node has already been loaded into the data textures.
        continue;
      }

      // Otherwise, check that the data textures have the space to add it.
      const UCesiumGltfVoxelComponent* pVoxel = currentTile.pComponent;
      size_t addNodeIndex = 0;
      if (this->_dataTextures.IsFull()) {
        addNodeIndex = existingNodeCount - 1 - destroyedNodeCount;
        if (addNodeIndex >= this->_loadedNodeIds.size()) {
          // This happens when all of the previously loaded nodes have been
          // replaced with new ones.
          continue;
        }

        destroyedNodeCount++;

        const CesiumGeometry::OctreeTileID& lowestPriorityId =
            this->_loadedNodeIds[addNodeIndex];
        FVoxelOctree::Node* pLowestPriorityNode =
            this->_octree.GetNode(lowestPriorityId);

        // Release the data slot of the lowest priority node.
        this->_dataTextures.Release(pLowestPriorityNode->DataSlotIndex);
        pLowestPriorityNode->DataSlotIndex = -1;

        // Attempt to remove the node and simplify the octree.
        // Will not succeed if the node's siblings are renderable, or if this
        // node contains renderable children.
        shouldUpdateOctree |= this->_octree.RemoveNode(lowestPriorityId);
      } else {
        addNodeIndex = existingNodeCount + addedNodeCount;
        addedNodeCount++;
      }

      // Create the node if it does not already exist in the tree.
      bool createdNewNode = this->_octree.CreateNode(currentTileId);
      pNode = this->_octree.GetNode(currentTileId);
      pNode->LastKnownScreenSpaceError = currentTile.sse;

      pNode->DataSlotIndex = this->_dataTextures.Add(*pVoxel);
      bool addedToDataTexture = (pNode->DataSlotIndex >= 0);
      shouldUpdateOctree |= createdNewNode || addedToDataTexture;

      if (!addedToDataTexture) {
        continue;
      } else if (addNodeIndex < this->_loadedNodeIds.size()) {
        this->_loadedNodeIds[addNodeIndex] = currentTileId;
      } else {
        this->_loadedNodeIds.push_back(currentTileId);
      }
    }
  } else {
    // If there are no data textures, then for all of the visible nodes...
    for (; !this->_visibleTileQueue.empty(); this->_visibleTileQueue.pop()) {
      const VoxelTileUpdateInfo& currentTile = this->_visibleTileQueue.top();
      const CesiumGeometry::OctreeTileID& currentTileId =
          currentTile.pComponent->tileId;
      // Create the node if it does not already exist in the tree.
      shouldUpdateOctree |= this->_octree.CreateNode(currentTileId);

      FVoxelOctree::Node* pNode = this->_octree.GetNode(currentTileId);
      pNode->LastKnownScreenSpaceError = currentTile.sse;
      // Set to arbitrary index. This will prompt the tile to render even though
      // it does not actually have data.
      pNode->DataSlotIndex = 0;
    }
  }

  if (shouldUpdateOctree) {
    this->_octree.UpdateTexture();
  }
}

double FVoxelResources::computePriority(double sse) {
  return 10.0 * sse / (sse + 1.0);
}
