// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "VoxelMegatextures.h"

#include "CesiumGltfVoxelComponent.h"
#include "CesiumLifetime.h"
#include "CesiumMetadataPropertyDetails.h"
#include "CesiumRuntime.h"
#include "CesiumTextureResource.h"
#include "CesiumVoxelMetadataComponent.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "EncodedFeaturesMetadata.h"
#include "EncodedMetadataConversions.h"
#include "Engine/VolumeTexture.h"
#include "Templates/UniquePtr.h"

#include <Cesium3DTiles/Class.h>
#include <CesiumGltf/PropertyType.h>
#include <glm/gtx/component_wise.hpp>

using namespace CesiumGltf;
using namespace EncodedFeaturesMetadata;

FVoxelMegatextures::FVoxelMegatextures(
    const FCesiumVoxelClassDescription& description,
    const glm::uvec3& slotDimensions,
    ERHIFeatureLevel::Type featureLevel,
    uint32 knownTileCount)
    : _slots(),
      _loadingSlots(),
      _pEmptySlotsHead(nullptr),
      _pOccupiedSlotsHead(nullptr),
      _slotDimensions(slotDimensions),
      _tileCountAlongAxes(0),
      _maximumTileCount(0),
      _propertyMap() {
  if (description.Properties.IsEmpty()) {
    return;
  }

  if (!RHISupportsVolumeTextures(featureLevel)) {
    // TODO: 2D fallback? Not sure if this check is the same as
    // SupportsVolumeTextureRendering, which is false on Vulkan Android, Metal,
    // and OpenGL.
    UE_LOG(
        LogCesium,
        Error,
        TEXT(
            "Volume textures are not supported. Unable to create the textures necessary for rendering voxels."))
    return;
  }

  // Attributes can take up varying texel sizes based on their type.
  // So first, identify which attribute is the largest in size.
  uint32 maximumTexelSizeBytes = 0;
  for (const FCesiumPropertyAttributePropertyDescription& Property :
       description.Properties) {
    EncodedPixelFormat encodedFormat = getPixelFormat(
        Property.EncodingDetails.Type,
        Property.EncodingDetails.ComponentType);
    if (encodedFormat.format == EPixelFormat::PF_Unknown) {
      continue;
    }

    uint32 texelSizeBytes =
        encodedFormat.channels * encodedFormat.bytesPerChannel;
    this->_propertyMap.Add(
        Property.Name,
        {encodedFormat, texelSizeBytes, nullptr});

    maximumTexelSizeBytes = FMath::Max(maximumTexelSizeBytes, texelSizeBytes);
  }

  if (maximumTexelSizeBytes == 0) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT(
            "No properties on UCesiumVoxelMetadataComponent are valid; none will be passed to the material."))
    return;
  }

  uint32 texelsPerSlot = glm::compMul(slotDimensions);
  uint32 memoryPerTexture = DefaultTextureMemoryBytes;
  if (knownTileCount > 0) {
    memoryPerTexture = glm::min(
        maximumTexelSizeBytes * texelsPerSlot * knownTileCount,
        MaximumTextureMemoryBytes);
  }

  uint32 maximumTexelCount = memoryPerTexture / maximumTexelSizeBytes;

  // Find a best fit for the requested memory. Given a target volume
  // (maximumTexelCount) and the slot dimensions (xyz), find some scalar that
  // fits the dimensions as close as possible.
  float scalar = std::cbrtf(float(maximumTexelCount) / float(texelsPerSlot));
  glm::vec3 textureDimensions = glm::round(scalar * glm::vec3(slotDimensions));

  this->_tileCountAlongAxes = glm::uvec3(textureDimensions) / slotDimensions;

  if (glm::any(glm::equal(this->_tileCountAlongAxes, glm::uvec3(0)))) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT(
            "Unable to create data textures for voxel dataset due to limited memory."))
    return;
  }

  glm::uvec3 actualDimensions = this->_tileCountAlongAxes * slotDimensions;

  this->_maximumTileCount = this->_tileCountAlongAxes.x *
                            this->_tileCountAlongAxes.y *
                            this->_tileCountAlongAxes.z;

  // Initialize the data slots.
  this->_slots.resize(this->_maximumTileCount, Slot());
  for (size_t i = 0; i < this->_slots.size(); i++) {
    Slot* pSlot = &_slots[i];
    pSlot->index = static_cast<int64_t>(i);
    pSlot->pPrevious = i > 0 ? &_slots[i - 1] : nullptr;
    pSlot->pNext = i < _slots.size() - 1 ? &_slots[i + 1] : nullptr;
  }

  this->_pEmptySlotsHead = &this->_slots[0];

  // Create the actual textures.
  for (auto& propertyIt : this->_propertyMap) {
    FTextureResource* pResource = FCesiumTextureResource::CreateEmpty(
                                      TextureGroup::TEXTUREGROUP_8BitData,
                                      actualDimensions.x,
                                      actualDimensions.y,
                                      actualDimensions.z,
                                      propertyIt.Value.encodedFormat.format,
                                      TextureFilter::TF_Nearest,
                                      TextureAddress::TA_Clamp,
                                      TextureAddress::TA_Clamp,
                                      false)
                                      .Release();

    UVolumeTexture* pTexture = NewObject<UVolumeTexture>(
        GetTransientPackage(),
        MakeUniqueObjectName(
            GetTransientPackage(),
            UTexture2D::StaticClass(),
            "CesiumVoxelDataTexture"),
        RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);
    pTexture->Filter = TextureFilter::TF_Nearest;
    pTexture->LODGroup = TextureGroup::TEXTUREGROUP_8BitData;
    pTexture->SRGB = false;
    pTexture->NeverStream = true;

    pTexture->SetResource(pResource);
    propertyIt.Value.pTexture = pTexture;

    ENQUEUE_RENDER_COMMAND(Cesium_InitResource)
    ([pTexture, pResource = pTexture->GetResource()](
         FRHICommandListImmediate& RHICmdList) {
      if (!pResource)
        return;

      pResource->SetTextureReference(
          pTexture->TextureReference.TextureReferenceRHI);
      pResource->InitResource(FRHICommandListImmediate::Get());
    });
  }
}

FVoxelMegatextures::~FVoxelMegatextures() {
  CESIUM_ASSERT(this->canBeDestroyed());
}

bool FVoxelMegatextures::canBeDestroyed() const {
  return this->_loadingSlots.size() == 0;
}

UTexture* FVoxelMegatextures::getTexture(const FString& attributeId) const {
  const TextureData* pProperty = this->_propertyMap.Find(attributeId);
  return pProperty ? pProperty->pTexture : nullptr;
}

/**
 * NOTE: This function assumes that the data being read from pData is the same
 * type that the texture expects. Coercive encoding behavior (similar to what
 * is done for CesiumPropertyTableProperty) could be added in the future.
 */
/*static*/ void FVoxelMegatextures::directCopyToTexture(
    const FCesiumPropertyAttributeProperty& property,
    const FVoxelMegatextures::TextureData& data,
    const FUpdateTextureRegion3D& updateRegion) {
  if (!data.pTexture)
    return;

  const uint8* pData =
      reinterpret_cast<const uint8*>(property.getAccessorData());

  ENQUEUE_RENDER_COMMAND(Cesium_DirectCopyVoxels)
  ([pTexture = data.pTexture,
    format = data.encodedFormat.format,
    &property,
    updateRegion,
    texelSizeBytes = data.texelSizeBytes,
    pData](FRHICommandListImmediate& RHICmdList) {
    FTextureResource* pResource =
        IsValid(pTexture) ? pTexture->GetResource() : nullptr;
    if (!pResource)
      return;

    // Pitch = size in bytes of each row of the source image.
    uint32 srcRowPitch = updateRegion.Width * texelSizeBytes;
    uint32 srcDepthPitch =
        updateRegion.Width * updateRegion.Height * texelSizeBytes;

    RHIUpdateTexture3D(
        pResource->TextureRHI,
        0,
        updateRegion,
        srcRowPitch,
        srcDepthPitch,
        pData);
  });
}

/*static*/ void FVoxelMegatextures::incrementalWriteToTexture(
    const FCesiumPropertyAttributeProperty& property,
    const FVoxelMegatextures::TextureData& data,
    const FUpdateTextureRegion3D& updateRegion) {
  if (!data.pTexture)
    return;

  ENQUEUE_RENDER_COMMAND(Cesium_IncrementalWriteVoxels)
  ([pTexture = data.pTexture,
    format = data.encodedFormat.format,
    &property,
    updateRegion,
    texelSizeBytes =
        data.texelSizeBytes](FRHICommandListImmediate& RHICmdList) {
    FTextureResource* pResource =
        IsValid(pTexture) ? pTexture->GetResource() : nullptr;
    if (!pResource)
      return;

    FUpdateTexture3DData UpdateData =
        RHIBeginUpdateTexture3D(pResource->TextureRHI, 0, updateRegion);

    for (uint32 z = 0; z < updateRegion.Depth; z++) {
      for (uint32 y = 0; y < updateRegion.Height; y++) {
        int64_t sourceIndex = int64_t(
            z * updateRegion.Width * updateRegion.Height +
            y * updateRegion.Width);
        uint8* pDestRow = UpdateData.Data + z * UpdateData.DepthPitch +
                          y * UpdateData.RowPitch;

        for (uint32 x = 0; x < updateRegion.Width; x++) {
          FCesiumMetadataValue rawValue =
              UCesiumPropertyAttributePropertyBlueprintLibrary::GetRawValue(
                  property,
                  sourceIndex++);

          float* pFloat =
              reinterpret_cast<float*>(pDestRow + x * texelSizeBytes);
          FMemory::Memcpy<float>(
              *pFloat,
              UCesiumMetadataValueBlueprintLibrary::GetFloat(rawValue, 0.0f));
        }
      }
    }

    RHIEndUpdateTexture3D(UpdateData);
  });
}

int64 FVoxelMegatextures::add(const UCesiumGltfVoxelComponent& voxelComponent) {
  int64 slotIndex = this->reserveNextSlot();
  if (slotIndex < 0) {
    return -1;
  }

  // Compute the update region for the data textures.
  FUpdateTextureRegion3D updateRegion;
  updateRegion.Width = this->_slotDimensions.x;
  updateRegion.Height = this->_slotDimensions.y;
  updateRegion.Depth = this->_slotDimensions.z;
  updateRegion.SrcX = 0;
  updateRegion.SrcY = 0;
  updateRegion.SrcZ = 0;

  uint32 zSlice = this->_tileCountAlongAxes.x * this->_tileCountAlongAxes.y;
  uint32 indexZ = slotIndex / zSlice;
  uint32 indexY = (slotIndex % zSlice) / this->_tileCountAlongAxes.x;
  uint32 indexX = slotIndex % this->_tileCountAlongAxes.x;
  updateRegion.DestZ = indexZ * this->_slotDimensions.z;
  updateRegion.DestY = indexY * this->_slotDimensions.y;
  updateRegion.DestX = indexX * this->_slotDimensions.x;

  uint32 index = static_cast<uint32>(slotIndex);

  for (const auto& PropertyIt : this->_propertyMap) {
    const FCesiumPropertyAttributeProperty& property =
        UCesiumPropertyAttributeBlueprintLibrary::FindProperty(
            voxelComponent.PropertyAttribute,
            PropertyIt.Key);

    if (UCesiumPropertyAttributePropertyBlueprintLibrary::
            GetPropertyAttributePropertyStatus(property) !=
        ECesiumPropertyAttributePropertyStatus::Valid) {
      continue;
    }

    if (property.getAccessorStride() == PropertyIt.Value.texelSizeBytes) {
      directCopyToTexture(property, PropertyIt.Value, updateRegion);
    } else {
      incrementalWriteToTexture(property, PropertyIt.Value, updateRegion);
    }
  }

  this->_slots[slotIndex].fence.emplace().BeginFence();
  this->_loadingSlots.insert(slotIndex);

  return slotIndex;
}

bool FVoxelMegatextures::release(int64_t slotIndex) {
  if (slotIndex < 0 || slotIndex >= int64(this->_slots.size())) {
    return false; // Index out of bounds
  }

  Slot* pSlot = &this->_slots[slotIndex];
  pSlot->fence.reset();

  if (pSlot->pPrevious) {
    pSlot->pPrevious->pNext = pSlot->pNext;
  }
  if (pSlot->pNext) {
    pSlot->pNext->pPrevious = pSlot->pPrevious;
  }

  // Move to list of empty slots (as the new head)
  pSlot->pNext = this->_pEmptySlotsHead;
  if (pSlot->pNext) {
    pSlot->pNext->pPrevious = pSlot;
  }

  pSlot->pPrevious = nullptr;
  this->_pEmptySlotsHead = pSlot;

  return true;
}

int64 FVoxelMegatextures::reserveNextSlot() {
  // Remove head from list of empty slots
  FVoxelMegatextures::Slot* pSlot = this->_pEmptySlotsHead;
  if (!pSlot) {
    return -1;
  }

  this->_pEmptySlotsHead = pSlot->pNext;

  if (this->_pEmptySlotsHead) {
    this->_pEmptySlotsHead->pPrevious = nullptr;
  }

  // Move to list of occupied slots (as the new head)
  pSlot->pNext = this->_pOccupiedSlotsHead;
  if (pSlot->pNext) {
    this->_pOccupiedSlotsHead->pPrevious = pSlot;
  }
  this->_pOccupiedSlotsHead = pSlot;

  return pSlot->index;
}

bool FVoxelMegatextures::isSlotLoaded(int64 index) const {
  if (index < 0 || index >= int64(this->_slots.size()))
    return false;

  return this->_slots[size_t(index)].fence &&
         this->_slots[size_t(index)].fence->IsFenceComplete();
}

bool FVoxelMegatextures::pollLoadingSlots() {
  size_t loadingSlotCount = this->_loadingSlots.size();
  std::erase_if(this->_loadingSlots, [thiz = this](size_t i) {
    return thiz->isSlotLoaded(i);
  });
  return loadingSlotCount != this->_loadingSlots.size();
}
