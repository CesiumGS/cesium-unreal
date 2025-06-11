// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "VoxelOctree.h"
#include "CesiumLifetime.h"
#include "CesiumRuntime.h"

#include <Cesium3DTilesContent/ImplicitTilingUtilities.h>
#include <glm/glm.hpp>

using namespace CesiumGeometry;
using namespace Cesium3DTilesContent;

/**
 * A Cesium texture resource that creates an initially empty `FRHITexture` for
 * FVoxelOctree.
 */
class FCesiumVoxelOctreeTextureResource : public FCesiumTextureResource {
public:
  FCesiumVoxelOctreeTextureResource(
      TextureGroup textureGroup,
      uint32 width,
      uint32 height,
      EPixelFormat format,
      TextureFilter filter,
      TextureAddress addressX,
      TextureAddress addressY,
      bool sRGB,
      uint32 extData);

protected:
  virtual FTextureRHIRef InitializeTextureRHI() override;
};

FCesiumVoxelOctreeTextureResource::FCesiumVoxelOctreeTextureResource(
    TextureGroup textureGroup,
    uint32 width,
    uint32 height,
    EPixelFormat format,
    TextureFilter filter,
    TextureAddress addressX,
    TextureAddress addressY,
    bool sRGB,
    uint32 extData)
    : FCesiumTextureResource(
          textureGroup,
          width,
          height,
          0,
          format,
          filter,
          addressX,
          addressY,
          sRGB,
          false,
          extData,
          true) {}

FTextureRHIRef FCesiumVoxelOctreeTextureResource::InitializeTextureRHI() {
  FRHIResourceCreateInfo createInfo{TEXT("FVoxelOctreeTextureResource")};
  createInfo.BulkData = nullptr;
  createInfo.ExtData = this->_platformExtData;

  ETextureCreateFlags textureFlags = TexCreate_ShaderResource;
  if (this->bSRGB) {
    textureFlags |= TexCreate_SRGB;
  }

  // Create a new 2D RHI texture, initially empty.
  return RHICreateTexture(
      FRHITextureCreateDesc::Create2D(createInfo.DebugName)
          .SetExtent(int32(this->_width), int32(this->_height))
          .SetFormat(this->_format)
          .SetNumMips(1)
          .SetNumSamples(1)
          .SetFlags(textureFlags)
          .SetInitialState(ERHIAccess::Unknown)
          .SetExtData(createInfo.ExtData)
          .SetGPUMask(createInfo.GPUMask)
          .SetClearValue(createInfo.ClearValueBinding));
}

void UCesiumVoxelOctreeTexture::update(const std::vector<std::byte>& data) {
  if (!this->_pResource || !this->_pResource->TextureRHI) {
    return;
  }

  // Compute the area of the texture that actually needs updating.
  uint32 texelCount = data.size() / sizeof(uint32);
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

  region.Width = FMath::Clamp(region.Width, 1, this->_pResource->GetSizeX());
  region.Height = FMath::Clamp(region.Height, 1, this->_pResource->GetSizeY());

  // Pitch = size in bytes of each row of the source image
  uint32 sourcePitch = region.Width * sizeof(uint32);
  ENQUEUE_RENDER_COMMAND(Cesium_UpdateResource)
  ([pResource = this->_pResource, &data, region, sourcePitch](
       FRHICommandListImmediate& RHICmdList) {
    RHIUpdateTexture2D(
        pResource->TextureRHI,
        0,
        region,
        sourcePitch,
        reinterpret_cast<const uint8*>(data.data()));
  });
}

/*static*/ UCesiumVoxelOctreeTexture*
UCesiumVoxelOctreeTexture::create(uint32 Width, uint32 MaximumTileCount) {
  uint32 TilesPerRow = Width / TexelsPerNode;
  float Height = (float)MaximumTileCount / (float)TilesPerRow;
  Height = static_cast<uint32>(FMath::CeilToInt64(Height));
  Height = FMath::Clamp(Height, 1, Width);

  TUniquePtr<FCesiumVoxelOctreeTextureResource> pResource =
      MakeUnique<FCesiumVoxelOctreeTextureResource>(
          TextureGroup::TEXTUREGROUP_8BitData,
          Width,
          Height,
          EPixelFormat::PF_R8G8B8A8,
          TextureFilter::TF_Nearest,
          TextureAddress::TA_Clamp,
          TextureAddress::TA_Clamp,
          false,
          0);

  UCesiumVoxelOctreeTexture* pTexture = NewObject<UCesiumVoxelOctreeTexture>(
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

  pTexture->_tilesPerRow = TilesPerRow;
  pTexture->_pResource = pResource.Release();
  pTexture->SetResource(pTexture->_pResource);

  return pTexture;
}

size_t FVoxelOctree::OctreeTileIDHash::operator()(
    const CesiumGeometry::OctreeTileID& tileId) const {
  // Tiles with the same morton index, but on different levels, are
  // distinguished by an offset. This offset is equal to the number of tiles on
  // the levels above it, which is the sum of a series, where n = tile.level -
  // 1:
  // 1 + 8 + 8^2 + ... + 8^n = (8^(n+1) - 1) / (8 - 1)
  // e.g., for TileID(2, 0, 0, 0), the morton index is 0, but the hash = 9.
  size_t levelOffset =
      tileId.level > 0 ? (std::pow(8, tileId.level) - 1) / 7 : 0;
  return levelOffset + ImplicitTilingUtilities::computeMortonIndex(tileId);
}

FVoxelOctree::FVoxelOctree() : _nodes() {
  CesiumGeometry::OctreeTileID rootTileID(0, 0, 0, 0);
  this->_nodes.insert({rootTileID, Node()});
}

FVoxelOctree::~FVoxelOctree() {
  std::vector<std::byte> empty;
  std::swap(this->_octreeData, empty);

  UTexture2D* pTexture = this->_pTexture;
  this->_pTexture = nullptr;

  if (IsValid(pTexture)) {
    pTexture->RemoveFromRoot();
    CesiumLifetime::destroy(pTexture);
  }
}

void FVoxelOctree::initializeTexture(uint32 Width, uint32 MaximumTileCount) {
  this->_pTexture = UCesiumVoxelOctreeTexture::create(Width, MaximumTileCount);
  FTextureResource* pResource =
      this->_pTexture ? this->_pTexture->GetResource() : nullptr;
  if (!pResource) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT("Could not create texture for voxel octree."));
    return;
  }

  ENQUEUE_RENDER_COMMAND(Cesium_InitResource)
  ([pTexture = this->_pTexture,
    pResource](FRHICommandListImmediate& RHICmdList) {
    pResource->SetTextureReference(
        pTexture->TextureReference.TextureReferenceRHI);
    pResource->InitResource(FRHICommandListImmediate::Get());
  });
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

  // Create this node first.
  this->_nodes.insert({TileID, Node()});
  pNode = &this->_nodes[TileID];

  // Starting from the target node, traverse the tree upwards and create the
  // missing nodes. Stop when we've found an existing parent node.
  CesiumGeometry::OctreeTileID currentTileID = TileID;
  bool foundExistingParent = false;
  for (uint32_t level = TileID.level; level > 0; level--) {
    CesiumGeometry::OctreeTileID parentTileID =
        *computeParentTileID(currentTileID);
    if (this->_nodes.find(parentTileID) == this->_nodes.end()) {
      // Parent doesn't exist, so create it.
      this->_nodes.insert({parentTileID, Node()});
    } else {
      foundExistingParent = true;
    }

    FVoxelOctree::Node* pParent = &this->_nodes[parentTileID];
    pNode->pParent = pParent;

    // The parent *shouldn't* have children at this point. Otherwise, our
    // target node would have already been found.
    std::array<CesiumGeometry::OctreeTileID, 8> childIds =
        computeChildTileIDs(parentTileID);
    for (CesiumGeometry::OctreeTileID& childId : childIds) {
      if (this->_nodes.find(childId) == this->_nodes.end()) {
        this->_nodes.insert({childId, Node()});
        this->_nodes[childId].pParent = pParent;
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

bool FVoxelOctree::removeNode(const CesiumGeometry::OctreeTileID& TileID) {
  if (TileID.level == 0) {
    return false;
  }

  if (isNodeRenderable(TileID)) {
    return false;
  }

  // Check the sibling nodes. If they are either leaves or have renderable
  // children, return true.
  bool hasRenderableSiblings = false;

  // There may be cases where the children rely on the parent for rendering.
  // If so, the node's data cannot be easily released.
  // TODO: can you also attempt to destroy the node?
  CesiumGeometry::OctreeTileID parentTileID = *computeParentTileID(TileID);
  std::array<CesiumGeometry::OctreeTileID, 8> siblingIds =
      computeChildTileIDs(parentTileID);
  for (const CesiumGeometry::OctreeTileID& siblingId : siblingIds) {
    if (siblingId == TileID)
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
  for (const CesiumGeometry::OctreeTileID& siblingId : siblingIds) {
    this->_nodes.erase(this->_nodes.find(siblingId));
  }
  this->getNode(parentTileID)->hasChildren = false;

  // Continue to recursively remove parent nodes as long as they aren't
  // renderable either.
  removeNode(parentTileID);

  return true;
}

bool FVoxelOctree::isNodeRenderable(
    const CesiumGeometry::OctreeTileID& TileID) const {
  const FVoxelOctree::Node* pNode = this->getNode(TileID);
  if (!pNode) {
    return false;
  }

  return pNode->dataSlotIndex >= 0 || pNode->hasChildren;
}

/*static*/ std::array<CesiumGeometry::OctreeTileID, 8>
FVoxelOctree::computeChildTileIDs(const CesiumGeometry::OctreeTileID& TileID) {
  uint32 level = TileID.level + 1;
  uint32 x = TileID.x << 1;
  uint32 y = TileID.y << 1;
  uint32 z = TileID.z << 1;

  return {
      OctreeTileID(level, x, y, z),
      OctreeTileID(level, x + 1, y, z),
      OctreeTileID(level, x, y + 1, z),
      OctreeTileID(level, x + 1, y + 1, z),
      OctreeTileID(level, x, y, z + 1),
      OctreeTileID(level, x + 1, y, z + 1),
      OctreeTileID(level, x, y + 1, z + 1),
      OctreeTileID(level, x + 1, y + 1, z + 1)};
}

/*static*/ std::optional<CesiumGeometry::OctreeTileID>
FVoxelOctree::computeParentTileID(const CesiumGeometry::OctreeTileID& TileID) {
  if (TileID.level == 0) {
    return std::nullopt;
  }
  return CesiumGeometry::OctreeTileID(
      TileID.level - 1,
      TileID.x >> 1,
      TileID.y >> 1,
      TileID.z >> 1);
}

/*static*/ void FVoxelOctree::insertNodeData(
    std::vector<std::byte>& nodeData,
    uint32 textureIndex,
    FVoxelOctree::ENodeFlag nodeFlag,
    uint16 data,
    uint8 renderableLevelDifference) {
  uint32 dataIndex = textureIndex * sizeof(uint32);
  if (nodeData.size() <= dataIndex) {
    nodeData.resize(dataIndex + sizeof(uint32), std::byte(0));
  }
  // Explicitly encode the values in little endian order.
  nodeData[dataIndex] = std::byte(nodeFlag);
  nodeData[dataIndex + 1] = std::byte(renderableLevelDifference);
  nodeData[dataIndex + 2] = std::byte(data & 0x00ff);
  nodeData[dataIndex + 3] = std::byte(data >> 8);
};

void FVoxelOctree::encodeNode(
    const CesiumGeometry::OctreeTileID& tileId,
    std::vector<std::byte>& nodeData,
    uint32& nodeCount,
    uint32 octreeIndex,
    uint32 textureIndex,
    uint32 parentOctreeIndex,
    uint32 parentTextureIndex) {
  constexpr uint32 TexelsPerNode = UCesiumVoxelOctreeTexture::TexelsPerNode;

  Node* pNode = &this->_nodes[tileId];
  if (pNode->hasChildren) {
    // Point the parent and child octree indices at each other
    insertNodeData(
        nodeData,
        parentTextureIndex,
        ENodeFlag::Internal,
        octreeIndex);
    insertNodeData(
        nodeData,
        textureIndex,
        ENodeFlag::Internal,
        parentOctreeIndex);
    nodeCount++;

    // Continue traversing
    parentOctreeIndex = octreeIndex;
    parentTextureIndex = parentOctreeIndex * TexelsPerNode + 1;

    std::array<CesiumGeometry::OctreeTileID, 8> childIds =
        computeChildTileIDs(tileId);
    for (uint32 i = 0; i < childIds.size(); i++) {
      octreeIndex = nodeCount;
      textureIndex = octreeIndex * TexelsPerNode;

      encodeNode(
          childIds[i],
          nodeData,
          nodeCount,
          octreeIndex,
          textureIndex,
          parentOctreeIndex,
          parentTextureIndex + i);
    }
  } else {
    // Leaf nodes involve more complexity.
    ENodeFlag flag = ENodeFlag::Empty;
    uint16 value = 0;
    uint16 levelDifference = 0;

    if (pNode->dataSlotIndex >= 0) {
      flag = ENodeFlag::Leaf;
      value = static_cast<uint16>(pNode->dataSlotIndex);
    } else if (pNode->pParent) {
      FVoxelOctree::Node* pParent = pNode->pParent;

      for (uint32 levelsAbove = 1; levelsAbove <= tileId.level; levelsAbove++) {
        if (pParent->dataSlotIndex >= 0) {
          flag = ENodeFlag::Leaf;
          value = static_cast<uint16>(pParent->dataSlotIndex);
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
    insertNodeData(nodeData, parentTextureIndex, flag, value, levelDifference);
    nodeCount++;
  }
}

void FVoxelOctree::updateTexture() {
  if (!this->_pTexture) {
    return;
  }

  this->_octreeData.clear();
  uint32_t nodeCount = 0;
  encodeNode(
      CesiumGeometry::OctreeTileID(0, 0, 0, 0),
      this->_octreeData,
      nodeCount,
      0,
      0,
      0,
      0);

  // Pad the data as necessary for the texture copy.
  uint32 regionWidth = this->_pTexture->getTilesPerRow() *
                       UCesiumVoxelOctreeTexture::TexelsPerNode *
                       sizeof(uint32);
  uint32 regionHeight =
      glm::ceil((float)this->_octreeData.size() / regionWidth);
  uint32 expectedSize = regionWidth * regionHeight;
  if (this->_octreeData.size() != expectedSize) {
    this->_octreeData.resize(expectedSize, std::byte(0));
  }

  this->_pTexture->update(this->_octreeData);
}
