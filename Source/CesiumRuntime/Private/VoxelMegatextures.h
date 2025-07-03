// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumCommon.h"
#include "CesiumMetadataValueType.h"
#include "EncodedFeaturesMetadata.h"
#include "RenderCommandFence.h"

#include <CesiumGltf/PropertyType.h>
#include <glm/glm.hpp>
#include <unordered_set>

struct FCesiumVoxelClassDescription;
class FCesiumTextureResource;
class UCesiumGltfVoxelComponent;

/**
 * Data texture resources for a voxel dataset, with one texture per voxel
 * attribute. A data texture is a "megatexture" containing numerous slots, each
 * of which can store the data of one voxel primitive. This is responsible for
 * synchronizing which slots are occupied across all data textures.
 *
 * Due to the requirements of voxel rendering (primarily, sampling voxels from
 * neighboring tiles), the voxels within a tileset are drawn in a single pass.
 * This texture manages all of the currently-loaded voxel data and is itself
 * passed to the material.
 *
 * Counterpart to Megatexture.js in CesiumJS, except this takes advantage of 3D
 * textures to simplify some of the texture read/write math.
 */
class FVoxelMegatextures {
public:
  /**
   * @brief Constructs a set of voxel data textures.
   *
   * @param description The voxel class description, indicating which metadata
   * attributes to encode.
   * @param slotDimensions The dimensions of each slot (i.e, the voxel grid
   * dimensions, including padding).
   * @param featureLevel The RHI feature level associated with the scene.
   * @param knownTileCount The number of known tiles in the tileset. This
   * informs how much texture memory will be allocated for the data textures. If
   * this is zero, a default value will be used.
   */
  FVoxelMegatextures(
      const FCesiumVoxelClassDescription& description,
      const glm::uvec3& slotDimensions,
      ERHIFeatureLevel::Type featureLevel,
      uint32 knownTileCount);

  ~FVoxelMegatextures();

  /**
   * @brief Gets the maximum number of tiles that can be added to the data
   * textures. Equivalent to the maximum number of data slots.
   */
  uint32 getMaximumTileCount() const {
    return static_cast<uint32>(this->_slots.size());
  }

  /**
   * @brief Gets the number of tiles along each dimension of the textures.
   */
  glm::uvec3 getTileCountAlongAxes() const { return this->_tileCountAlongAxes; }

  /**
   * @brief Retrieves the texture containing the data for the attribute with
   * the given ID. Returns nullptr if the attribute does not exist.
   */
  UTexture* getTexture(const FString& attributeId) const;

  /**
   * @brief Whether or not all slots in the textures are occupied.
   */
  bool isFull() const { return this->_pEmptySlotsHead == nullptr; }

  /**
   * @brief Attempts to add the voxel tile to the data textures.
   *
   * @returns The index of the reserved slot, or -1 if none were available.
   */
  int64_t add(const UCesiumGltfVoxelComponent& voxelComponent);

  /**
   * @brief Releases the slot at the specified index, making the space available
   * for another voxel tile.
   */
  bool release(int64_t slotIndex);

  /**
   * @brief Whether or not the slot at the given index has loaded data.
   */
  bool isSlotLoaded(int64 index) const;

  /**
   * @brief Checks the progress of slots with data being loaded into the
   * megatexture. Retusn true if any slots completed loading.
   */
  bool pollLoadingSlots();

  /**
   * @brief Whether the textures can be destroyed. Returns false if there are
   * any render thread commands in flight.
   */
  bool canBeDestroyed() const;

private:
  /**
   * Value constants taken from CesiumJS.
   */
  static const uint32 MaximumTextureMemoryBytes = 512 * 1024 * 1024;
  static const uint32 DefaultTextureMemoryBytes = 128 * 1024 * 1024;

  /**
   * @brief Represents a slot in the voxel data texture that contains a single
   * tile's data. Slots function like nodes in a linked list in order to track
   * which slots are occupied with data, while preventing the need for 2 vectors
   * with maximum tile capacity.
   */
  struct Slot {
    int64_t index = -1;
    Slot* pNext = nullptr;
    Slot* pPrevious = nullptr;
    std::optional<FRenderCommandFence> fence;
  };

  struct TextureData {
    /**
     * @brief The texture format used to store encoded property values.
     */
    EncodedFeaturesMetadata::EncodedPixelFormat encodedFormat;

    /**
     * @brief The size of a texel in the texture, in bytes. Derived from the
     * texture format.
     */
    uint32 texelSizeBytes;

    /**
     * @brief The data texture for this property.
     */
    UTexture* pTexture;
  };

  /**
   * @brief Directly copies the buffer from the given property attribute
   * property to the texture. This is much faster than
   * incrementalWriteToTexture, but requires the attribute data to be
   * contiguous.
   */
  static void directCopyToTexture(
      const FCesiumPropertyAttributeProperty& property,
      const TextureData& data,
      const FUpdateTextureRegion3D& region);

  /**
   * @brief Incrementally writes the data from the given property attribute
   * property to the texture. Each element is converted to uint8 or float,
   * depending on the texture format, before being written to the pixel.
   * Necessary for some accessor types or accessors with non-continguous data.
   */
  static void incrementalWriteToTexture(
      const FCesiumPropertyAttributeProperty& property,
      const TextureData& data,
      const FUpdateTextureRegion3D& region);

  /**
   * @brief Reserves the next available empty slot.
   *
   * @returns The index of the reserved slot, or -1 if none were available.
   */
  int64_t reserveNextSlot();

  std::vector<Slot> _slots;
  std::unordered_set<size_t> _loadingSlots;

  Slot* _pEmptySlotsHead;
  Slot* _pOccupiedSlotsHead;

  glm::uvec3 _slotDimensions;
  glm::uvec3 _tileCountAlongAxes;
  uint32 _maximumTileCount;

  TMap<FString, TextureData> _propertyMap;
};
