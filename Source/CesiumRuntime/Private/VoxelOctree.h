// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumTextureResource.h"
#include "Engine/Texture2D.h"
#include "RenderCommandFence.h"

#include <CesiumGeometry/OctreeTileID.h>
#include <optional>
#include <unordered_map>

/**
 * A texture that encodes information from \FVoxelOctree.
 */
class UCesiumVoxelOctreeTexture : public UTexture2D {
public:
  /**
   * @brief The number of texels used to represent a node in the texture.
   *
   * The first texel stores an index to the node's parent. The remaining eight
   * represent the indices of the node's children.
   */
  static const uint32 TexelsPerNode = 9;

  /**
   * @brief The maximum allowed width for the texture. Value taken from
   * CesiumJS.
   */
  static const uint32 MaximumOctreeTextureWidth = 2048;

  /**
   * @brief Creates a new texture with the specified tile capacity.
   */
  static UCesiumVoxelOctreeTexture* create(uint32 MaximumTileCount);

  /**
   * @brief Gets the number of tiles encoded in a single row of the texture.
   */
  uint32_t getTilesPerRow() const { return this->_tilesPerRow; }

  /**
   * @brief Updates the octree texture with the new input data.
   *
   * @returns True if the update succeeded, false otherwise.
   */
  bool update(const std::vector<std::byte>& data);

private:
  FCesiumTextureResourceUniquePtr _pResource;
  uint32_t _tilesPerRow;
};

/**
 * @brief A representation of an implicit octree tileset containing voxels.
 *
 * This is relevant to the raycasted approach for rendering voxels and
 * is meant to be paired with \ref FVoxelDataTextures. The structure of
 * the voxel tileset is communicated to the shader through a texture.
 * Tiles with renderable data are linked to slots in \ref FVoxelDataTextures.
 *
 * The connection with \ref FVoxelDataTextures is managed externally by
 * \UCesiumVoxelRendererComponent.
 */
class FVoxelOctree {
public:
  /**
   * @brief A tile in an implicitly tiled octree.
   */
  struct Node {
    /**
     * @brief Points to the parent of the node, if it exists.
     */
    Node* pParent = nullptr;
    /**
     * @brief Whether the tile's children exist in the octree.
     */
    bool hasChildren = false;
    /**
     * @brief The tile's last known screen space error.
     */
    double lastKnownScreenSpaceError = 0.0;
    /**
     * @brief The index of the slot that this tile occupies in \ref
     * VoxelDataTextures, if any.
     */
    int64_t dataIndex = -1;
    bool isDataReady = false;
  };

  /**
   * @brief Constructs an initially empty octree with the specified tile
   * capacity.
   */
  FVoxelOctree(uint32 maximumTileCount);

  ~FVoxelOctree();

  /**
   * @brief Gets a node in the octree at the specified tile ID. Returns nullptr
   * if it does not exist.
   *
   * @param TileID The octree tile ID.
   */
  const Node* getNode(const CesiumGeometry::OctreeTileID& TileID) const;

  /**
   * @copydoc getNode
   */
  Node* getNode(const CesiumGeometry::OctreeTileID& TileID);

  /**
   * @brief Creates a node in the octree at the specified tile ID, including the
   * parent nodes needed to traverse to it.
   *
   * If the node already exists, this returns false.
   *
   * @param TileID The octree tile ID.
   * @param Whether the node was successfully added.
   */
  bool createNode(const CesiumGeometry::OctreeTileID& TileID);

  /**
   * @brief Attempts to remove the node at the specified tile ID.
   *
   * This will fail to remove the node from the tree if:
   *
   * - the node is the root of the tree
   * - the node has renderable siblings
   *
   * @param TileID The octree tile ID.
   * @return Whether the node was successfully removed.
   */
  bool removeNode(const CesiumGeometry::OctreeTileID& TileID);

  /**
   * @brief Retrieves the texture containing the encoded octree.
   */
  UTexture2D* getTexture() const { return this->_pTexture; }

  bool updateTexture();

  bool canBeDestroyed() const;

private:
  /**
   * @brief An enum that indicates the type of a node encoded on the GPU.
   * Indicates what the numerical data value represents for that node.
   */
  enum class ENodeFlag : uint8 {
    /**
     * Empty leaf node that should be skipped when rendering.
     *
     * This may happen if a node's sibling is renderable, but neither it nor its
     * parent are renderable, which can happen Native's algorithm loads higher
     * LOD tiles before their ancestors.
     */
    Empty = 0,
    /**
     * Renderable leaf node with two possibilities:
     *
     * 1. The leaf node has its own data. The encoded data value refers to an
     * index in the data texture of the slot containing the voxel tile's data.
     *
     * 2. The leaf node has no data of its own but is forced to render (such as
     * when its siblings are renderable but it is not). The leaf will attempt to
     * render the data of the nearest ancestor. The encoded data value refers to
     * an index in the data texture of the slot containing the ancestor voxel
     * tile's data.
     *
     * The latter is a unique case that contains an extra packed value -- the
     * level difference from the nearest renderable ancestor. This is so the
     * rendering implementation can deduce the correct texture coordinates. If
     * the leaf node contains its own data, then this value is 0.
     */
    Leaf = 1,
    /**
     * Internal node. The encoded data value refers to an index in the octree
     * texture where its full representation is located.
     */
    Internal = 2,
  };

  /**
   * @brief Inserts the input values to the data vector, automatically
   * expanding it if the target index is out-of-bounds.
   */
  static void insertNodeData(
      std::vector<std::byte>& nodeData,
      uint32 textureIndex,
      ENodeFlag nodeFlag,
      uint16 data,
      uint8 renderableLevelDifference = 0);

  /**
   * @brief Recursively writes octree nodes as their expected representation
   * in the GPU texture.
   *
   * Example Below (shown as binary tree instead of octree for
   * demonstration purposes)
   *
   * Tree:
   *           0
   *          / \
   *         /   \
   *        /     \
   *       1       3
   *      / \     / \
   *     L0  2   L3 L4
   *        / \
   *       L1 L2
   *
   *
   * GPU Array:
   * L = leaf index
   * * = index to parent node
   * node index:   0_______  1________  2________  3_________
   * data array:  [*0, 1, 3, *0, L0, 2, *1 L1, L2, *0, L3, L4]
   *
   * The array is generated from a depth-first traversal. The end result could
   * be an unbalanced tree, so the parent index is stored at each node to make
   * it possible to traverse upwards.
   *
   * Nodes are indexed by the order in which they appear in the traversal.
   *
   * @param pNode The node to be encoded.
   * @param nodeData The data buffer to write to.
   * @param nodeCount The current number of encoded numbers, used to assign
   * indices to each node. Accumulates over all calls of this function.
   * @param octreeIndex The index of the node relative to all the nodes
   * encountered in the octree, based on nodeCount. Acts as the identifier for
   * the node.
   * @param textureIndex The texel index of the node within the texture where
   * the node will be expanded if it is an internal node.
   * @param parentOctreeIndex The octree index of the parent.
   * @param parentTextureIndex The texel index of the node within the texture
   * where the node data will be written, relative to the parent's texel
   * index.
   */
  void encodeNode(
      const CesiumGeometry::OctreeTileID& tileId,
      std::vector<std::byte>& nodeData,
      uint32& nodeCount,
      uint32 octreeIndex,
      uint32 textureIndex,
      uint32 parentOctreeIndex,
      uint32 parentTextureIndex);

  bool isNodeRenderable(const CesiumGeometry::OctreeTileID& TileID) const;

  struct OctreeTileIDHash {
    size_t operator()(const CesiumGeometry::OctreeTileID& tileId) const;
  };

  /**
   * This implementation is inspired by Linear (hashed) Octrees:
   * https://geidav.wordpress.com/2014/08/18/advanced-octrees-2-node-representations/
   *
   * Nodes must track their parent / child relationships so that the tree
   * structure can be encoded to a texture, for voxel raymarching. However,
   * nodes must also be easily created and/or accessed. cesium-native passes
   * tiles over in a vector without spatial organization. Typical tree
   * queries are O(log(n)) where n = # tree levels. This is unideal, since it's
   * likely that multiple tiles will be made visible in an update based on
   * movement.
   *
   * The compromise: a hashmap that stores octree nodes based on their tile ID.
   * The nodes don't point to any children themselves; instead, they store a
   * bool indicating whether or not children have been created for them. It's on
   * the octree to properly manage this.
   */
  using NodeMap =
      std::unordered_map<CesiumGeometry::OctreeTileID, Node, OctreeTileIDHash>;
  NodeMap _nodes;

  UCesiumVoxelOctreeTexture* _pTexture;
  std::optional<FRenderCommandFence> _fence;

  // As the octree grows, save the allocated memory so that recomputing the
  // same-size octree won't require more allocations.
  std::vector<std::byte> _data;
};
