// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "VoxelOctree.h"
#include "CesiumLifetime.h"
#include "CesiumRuntime.h"
#include "CesiumTextureResource.h"

#include <Cesium3DTilesContent/ImplicitTilingUtilities.h>
#include <glm/glm.hpp>

using namespace CesiumGeometry;
using namespace Cesium3DTilesContent;

/*static*/ UVoxelOctreeTexture*
UVoxelOctreeTexture::create(uint32 maximumTileCount) {
  const uint32 width = MaximumOctreeTextureWidth;
  uint32 tilesPerRow = width / TexelsPerNode;
  float height = (float)maximumTileCount / (float)tilesPerRow;
  height = static_cast<uint32>(FMath::CeilToInt64(height));
  height = FMath::Clamp(height, 1, MaximumOctreeTextureWidth);

  FTextureResource* pResource = FCesiumTextureResource::CreateEmpty(
                                    TextureGroup::TEXTUREGROUP_8BitData,
                                    MaximumOctreeTextureWidth,
                                    height,
                                    1, /* Depth */
                                    EPixelFormat::PF_R8G8B8A8,
                                    TextureFilter::TF_Nearest,
                                    TextureAddress::TA_Clamp,
                                    TextureAddress::TA_Clamp,
                                    false)
                                    .Release();

  UVoxelOctreeTexture* pTexture = NewObject<UVoxelOctreeTexture>(
      GetTransientPackage(),
      MakeUniqueObjectName(
          GetTransientPackage(),
          UTexture2D::StaticClass(),
          "VoxelOctreeTexture"),
      RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);

  pTexture->AddressX = TextureAddress::TA_Clamp;
  pTexture->AddressY = TextureAddress::TA_Clamp;
  pTexture->Filter = TextureFilter::TF_Nearest;
  pTexture->LODGroup = TextureGroup::TEXTUREGROUP_8BitData;
  pTexture->SRGB = false;
  pTexture->NeverStream = true;

  if (!pTexture || !pResource) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT("Could not create texture for voxel octree."));
    return nullptr;
  }

  pTexture->SetResource(pResource);
  pTexture->_tilesPerRow = tilesPerRow;

  ENQUEUE_RENDER_COMMAND(Cesium_InitResource)
  ([pTexture,
    pResource = pTexture->GetResource()](FRHICommandListImmediate& RHICmdList) {
    pResource->SetTextureReference(
        pTexture->TextureReference.TextureReferenceRHI);
    pResource->InitResource(FRHICommandListImmediate::Get());
  });

  return pTexture;
}

void UVoxelOctreeTexture::update(
    const FVoxelOctree& octree,
    std::vector<std::byte>& result) {
  result.clear();

  uint32_t nodeCount = 0;
  encodeNode(
      octree,
      CesiumGeometry::OctreeTileID(0, 0, 0, 0),
      nodeCount,
      0, /* octreeIndex */
      0, /* textureIndex */
      0, /* parentOctreeIndex */
      0,
      result); /* parentTextureIndex */

  // Pad the data as necessary for the texture copy.
  uint32 regionWidth = this->_tilesPerRow * TexelsPerNode * sizeof(uint32);
  uint32 regionHeight = glm::ceil((float)result.size() / regionWidth);
  uint32 expectedSize = regionWidth * regionHeight;

  if (result.size() != expectedSize) {
    result.resize(expectedSize, std::byte(0));
  }

  // Compute the area of the texture that actually needs updating.
  uint32 texelCount = result.size() / sizeof(uint32);
  uint32 tileCount = texelCount / TexelsPerNode;

  glm::uvec2 updateExtent;
  if (tileCount <= this->_tilesPerRow) {
    updateExtent.x = texelCount;
    updateExtent.y = 1;
  } else {
    updateExtent.x = this->_tilesPerRow * TexelsPerNode;
    updateExtent.y = texelCount / updateExtent.x;
  }
  updateExtent = glm::max(updateExtent, glm::uvec2(1, 1));

  FUpdateTextureRegion2D region;
  region.DestX = 0;
  region.DestY = 0;
  region.Width = updateExtent.x;
  region.Height = updateExtent.y;
  region.SrcX = 0;
  region.SrcY = 0;

  region.Width = FMath::Clamp(region.Width, 1, this->GetResource()->GetSizeX());
  region.Height =
      FMath::Clamp(region.Height, 1, this->GetResource()->GetSizeY());

  // Pitch = size in bytes of each row of the source image
  uint32 sourcePitch = region.Width * sizeof(uint32);

  ENQUEUE_RENDER_COMMAND(Cesium_UpdateResource)
  ([pResource = this->GetResource(), &result, region, sourcePitch](
       FRHICommandListImmediate& RHICmdList) {
    RHIUpdateTexture2D(
        pResource->TextureRHI,
        0,
        region,
        sourcePitch,
        reinterpret_cast<const uint8*>(result.data()));
  });
}

void UVoxelOctreeTexture::insertNodeData(
    std::vector<std::byte>& data,
    uint32 textureIndex,
    ENodeFlag nodeFlag,
    uint16 dataValue,
    uint8 renderableLevelDifference) {
  uint32_t dataIndex = textureIndex * sizeof(uint32_t);

  const size_t desiredSize = dataIndex + sizeof(uint32_t);
  if (data.size() < desiredSize) {
    data.resize(desiredSize);
  }

  // Explicitly encode the values in little endian order.
  data[dataIndex] = std::byte(nodeFlag);
  data[dataIndex + 1] = std::byte(renderableLevelDifference);
  data[dataIndex + 2] = std::byte(dataValue & 0x00ff);
  data[dataIndex + 3] = std::byte(dataValue >> 8);
};

void UVoxelOctreeTexture::encodeNode(
    const FVoxelOctree& octree,
    const CesiumGeometry::OctreeTileID& tileId,
    uint32& nodeCount,
    uint32 octreeIndex,
    uint32 textureIndex,
    uint32 parentOctreeIndex,
    uint32 parentTextureIndex,
    std::vector<std::byte>& result) {
  const FVoxelOctree::Node* pNode = octree.getNode(tileId);
  CESIUM_ASSERT(pNode);

  if (pNode->hasChildren) {
    // Point the parent and child octree indices at each other
    insertNodeData(
        result,
        parentTextureIndex,
        ENodeFlag::Internal,
        octreeIndex);
    insertNodeData(
        result,
        textureIndex,
        ENodeFlag::Internal,
        parentOctreeIndex);
    nodeCount++;

    // Continue traversing
    parentOctreeIndex = octreeIndex;
    parentTextureIndex = parentOctreeIndex * TexelsPerNode + 1;

    uint32 childIndex = 0;
    for (const CesiumGeometry::OctreeTileID& childId :
         ImplicitTilingUtilities::getChildren(tileId)) {
      octreeIndex = nodeCount;
      textureIndex = octreeIndex * TexelsPerNode;

      encodeNode(
          octree,
          childId,
          nodeCount,
          octreeIndex,
          textureIndex,
          parentOctreeIndex,
          parentTextureIndex + childIndex++,
          result);
    }
  } else {
    // Leaf nodes involve more complexity.
    ENodeFlag flag = ENodeFlag::Empty;
    uint16 value = 0;
    uint16 levelDifference = 0;

    if (pNode->isDataReady) {
      flag = ENodeFlag::Leaf;
      value = static_cast<uint16>(pNode->dataIndex);
    } else if (pNode->pParent) {
      FVoxelOctree::Node* pParent = pNode->pParent;

      for (uint32 levelsAbove = 1; levelsAbove <= tileId.level; levelsAbove++) {
        if (pParent->isDataReady) {
          flag = ENodeFlag::Leaf;
          value = static_cast<uint16>(pParent->dataIndex);
          levelDifference = levelsAbove;
          break;
        }

        // Continue trying to find a renderable ancestor.
        pParent = pParent->pParent;

        if (!pParent) {
          // This happens if we've reached the root node and it's not
          // renderable.
          break;
        }
      }
    }
    insertNodeData(result, parentTextureIndex, flag, value, levelDifference);
    nodeCount++;
  }
}

size_t FVoxelOctree::OctreeTileIDHash::operator()(
    const CesiumGeometry::OctreeTileID& tileId) const {
  // Tiles with the same morton index on different levels are distinguished by
  // an offset. This offset is equal to the total number of tiles on the levels
  // above it, i.e., the sum of a series where n = tile.level - 1:
  // 1 + 8 + 8^2 + ... + 8^n = (8^(n+1) - 1) / (8 - 1)
  // For example, TileID(2, 0, 0, 0) has a morton index of 0, but it hashes
  // to 9.
  size_t levelOffset =
      tileId.level > 0 ? (std::pow(8, tileId.level) - 1) / 7 : 0;
  return levelOffset + ImplicitTilingUtilities::computeMortonIndex(tileId);
}

FVoxelOctree::FVoxelOctree(uint32 maximumTileCount)
    : _nodes(), _pTexture(nullptr), _fence(std::nullopt), _data() {
  CesiumGeometry::OctreeTileID rootTileID(0, 0, 0, 0);
  this->_nodes.insert({rootTileID, Node()});
  this->_pTexture = UVoxelOctreeTexture::create(maximumTileCount);
}

FVoxelOctree::~FVoxelOctree() {
  CESIUM_ASSERT(!this->_fence || this->_fence->IsFenceComplete());
}

const FVoxelOctree::Node*
FVoxelOctree::getNode(const CesiumGeometry::OctreeTileID& TileID) const {
  return this->_nodes.contains(TileID) ? &this->_nodes.at(TileID) : nullptr;
}

FVoxelOctree::Node*
FVoxelOctree::getNode(const CesiumGeometry::OctreeTileID& TileID) {
  return this->_nodes.contains(TileID) ? &this->_nodes.at(TileID) : nullptr;
}

bool FVoxelOctree::createNode(const CesiumGeometry::OctreeTileID& TileID) {
  FVoxelOctree::Node* pNode = this->getNode(TileID);
  if (pNode) {
    return false;
  }

  // Create the target node first.
  this->_nodes.insert({TileID, Node()});
  pNode = &this->_nodes[TileID];

  // Starting from the target node, traverse the tree upwards and create the
  // missing ancestors. Stop when we've found an existing parent node.
  OctreeTileID currentTileID = TileID;
  bool foundExistingParent = false;

  for (uint32_t level = TileID.level; level > 0; level--) {
    OctreeTileID parentTileID =
        *ImplicitTilingUtilities::getParentID(currentTileID);
    if (this->_nodes.contains(parentTileID)) {
      foundExistingParent = true;
    } else {
      this->_nodes.insert({parentTileID, Node()});
    }

    FVoxelOctree::Node* pParent = &this->_nodes[parentTileID];
    pNode->pParent = pParent;

    // The parent *shouldn't* have children at this point. Otherwise, our
    // target node would have already been found.
    for (const CesiumGeometry::OctreeTileID& child :
         ImplicitTilingUtilities::getChildren(parentTileID)) {
      if (!this->_nodes.contains(child)) {
        this->_nodes.insert({child, Node()});
        this->_nodes[child].pParent = pParent;
      }
    }

    pParent->hasChildren = true;
    if (foundExistingParent) {
      // The parent already existed in the tree previously, no need to create
      // its ancestors.
      break;
    }

    currentTileID = parentTileID;
    pNode = pParent;
  }

  return true;
}

bool FVoxelOctree::removeNode(const CesiumGeometry::OctreeTileID& tileId) {
  if (tileId.level == 0) {
    return false;
  }

  if (isNodeRenderable(tileId)) {
    return false;
  }

  // Check the sibling nodes. If they are either leaves or have renderable
  // children, return true.
  bool hasRenderableSiblings = false;

  // There may be cases where the children rely on the parent for rendering.
  // If so, the node's data cannot be easily released.
  // TODO: can you also attempt to destroy the node?
  OctreeTileID parentTileId = *ImplicitTilingUtilities::getParentID(tileId);
  OctreeChildren siblings = ImplicitTilingUtilities::getChildren(parentTileId);
  for (const OctreeTileID& siblingId : siblings) {
    if (siblingId == tileId)
      continue;

    if (isNodeRenderable(siblingId)) {
      hasRenderableSiblings = true;
      break;
    }
  }

  if (hasRenderableSiblings) {
    // Don't remove this node yet. It will have to rely on its parent for
    // rendering.
    return false;
  }

  // Otherwise, okay to remove the nodes.
  for (const OctreeTileID& siblingId : siblings) {
    this->_nodes.erase(this->_nodes.find(siblingId));
  }
  this->getNode(parentTileId)->hasChildren = false;

  // Continue to recursively remove parent nodes as long as they aren't
  // renderable either.
  removeNode(parentTileId);

  return true;
}

bool FVoxelOctree::isNodeRenderable(
    const CesiumGeometry::OctreeTileID& TileID) const {
  const FVoxelOctree::Node* pNode = this->getNode(TileID);
  if (!pNode) {
    return false;
  }

  return pNode->dataIndex > 0 || pNode->hasChildren;
}

bool FVoxelOctree::updateTexture() {
  if (!this->_pTexture || (this->_fence && !this->_fence->IsFenceComplete())) {
    return false;
  }

  this->_fence.reset();
  this->_pTexture->update(*this, this->_data);

  // Prevent changes to the data while the texture is updating on the render
  // thread.
  this->_fence.emplace().BeginFence();
  return true;
}

bool FVoxelOctree::canBeDestroyed() const {
  return this->_fence ? this->_fence->IsFenceComplete() : true;
}
