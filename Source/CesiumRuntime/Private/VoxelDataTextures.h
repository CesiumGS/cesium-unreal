// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumCommon.h"
#include "CesiumMetadataValueType.h"
#include "EncodedFeaturesMetadata.h"
#include <CesiumGltf/PropertyType.h>

#include <glm/glm.hpp>

namespace Cesium3DTiles {
struct Class;
}

struct FCesiumVoxelClassDescription;
class FCesiumTextureResource;
class UCesiumGltfVoxelComponent;

/**
 * Manages the data texture resources for a voxel dataset, where
 * each data texture represents an attribute. This is responsible for
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
class FVoxelDataTextures {
public:
  /**
   * @brief Constructs a set of voxel data textures.
   *
   * @param pVoxelClass The voxel class description, indicating which metadata
   * attributes to encode.
   * @param dataDimensions The dimensions of the voxel data, including padding.
   * @param featureLevel The RHI feature level associated with the scene.
   * @param requestedMemoryPerTexture The requested texture memory for each
   * voxel attribute.
   */
  FVoxelDataTextures(
      const FCesiumVoxelClassDescription* pVoxelClass,
      const glm::uvec3& dataDimensions,
      ERHIFeatureLevel::Type featureLevel,
      uint32 requestedMemoryPerTexture);

  /**
   * Gets the maximum number of tiles that can be added to the data
   * textures. Equivalent to the maximum number of data slots.
   */
  uint32 GetMaximumTileCount() const {
    return static_cast<uint32>(this->_slots.size());
  }

  /**
   * Gets the number of tiles along each dimension of the textures.
   */
  glm::uvec3 GetTileCountAlongAxes() const { return this->_tileCountAlongAxes; }

  /**
   * Retrieves the texture containing the data for the attribute with
   * the given ID. Returns nullptr if the attribute does not exist.
   */
  UTexture* GetDataTexture(const FString& attributeId) const {
    const Property* pProperty = this->_propertyMap.Find(attributeId);
    if (pProperty) {
      return pProperty->pTexture;
    }
    return nullptr;
  }

  /**
   * @brief Retrieves how many data textures exist.
   */
  int32 GetTextureCount() const { return this->_propertyMap.Num(); }

  /**
   * Whether or not all slots in the textures are occupied.
   */
  bool IsFull() const { return this->_pEmptySlotsHead == nullptr; }

  /**
   * Attempts to add the voxel tile to the data textures.
   *
   * @returns The index of the reserved slot, or -1 if none were available.
   */
  int64_t Add(const UCesiumGltfVoxelComponent& voxelComponent);

  /**
   * Reserves the next available empty slot.
   *
   * @returns The index of the reserved slot, or -1 if none were available.
   */
  int64 ReserveNextSlot();

  /**
   * Releases the slot at the specified index, making the space available for
   * another voxel tile.
   */
  bool Release(uint32 slotIndex);

private:
  /**
   * Represents a slot in the voxel data texture that contains a single
   * tile's data. Slots function like nodes in a linked list in order to track
   * which slots are occupied with data, while preventing the need for 2 vectors
   * with maximum tile capacity.
   */
  struct Slot {
    int64 Index = -1;
    Slot* Next = nullptr;
    Slot* Previous = nullptr;
  };

  struct Property {
    /**
     * The texture format used to store encoded property values.
     */
    EncodedFeaturesMetadata::EncodedPixelFormat encodedFormat;

    // TODO: have to check RHISupportsVolumeTextures(GetFeatureLevel())
    // not sure if same as SupportsVolumeTextureRendering, which is false on
    // Vulkan Android, Metal, and OpenGL
    UTexture* pTexture;

    /**
     * A pointer to the texture resource. There is no way to retrieve this
     * through the UTexture API, so the pointer is stored here.
     */
    FCesiumTextureResource* pResource;
  };

  std::vector<Slot> _slots;
  Slot* _pEmptySlotsHead;
  Slot* _pOccupiedSlotsHead;

  glm::uvec3 _dataDimensions;
  glm::uvec3 _tileCountAlongAxes;
  uint32 _maximumTileCount;

  TMap<FString, Property> _propertyMap;
};
