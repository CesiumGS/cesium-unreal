// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "VoxelDataTextures.h"

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

using namespace CesiumGltf;
using namespace EncodedFeaturesMetadata;

FVoxelDataTextures::FVoxelDataTextures(
    const FCesiumVoxelClassDescription* pVoxelClass,
    const glm::uvec3& dataDimensions,
    ERHIFeatureLevel::Type featureLevel,
    uint32 requestedMemoryPerTexture)
    : _slots(),
      _loadingSlots(),
      _pEmptySlotsHead(nullptr),
      _pOccupiedSlotsHead(nullptr),
      _dataDimensions(dataDimensions),
      _tileCountAlongAxes(0),
      _maximumTileCount(0),
      _propertyMap() {
  if (!pVoxelClass) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "Voxel tileset is missing a UCesiumVoxelMetadataComponent. Add a UCesiumVoxelMetadataComponent to visualize the metadata within the tileset."))
    return;
  }

  if (pVoxelClass->Properties.IsEmpty()) {
    return;
  }

  if (!RHISupportsVolumeTextures(featureLevel)) {
    // TODO: 2D fallback? Not sure if this check is the same as
    // SupportsVolumeTextureRendering, which is false on Vulkan Android, Metal,
    // and OpenGL
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
       pVoxelClass->Properties) {
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
        {encodedFormat, texelSizeBytes, nullptr, nullptr});

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

  uint32 texelCount = requestedMemoryPerTexture / maximumTexelSizeBytes;
  uint32 textureDimension = std::cbrtf(static_cast<float>(texelCount));

  this->_tileCountAlongAxes =
      glm::uvec3(textureDimension) / this->_dataDimensions;

  if (this->_tileCountAlongAxes.x == 0 || this->_tileCountAlongAxes.y == 0 ||
      this->_tileCountAlongAxes.z == 0) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT(
            "Unable to create data textures for voxel dataset due to limited memory."))
    return;
  }

  glm::uvec3 actualDimensions =
      this->_tileCountAlongAxes * this->_dataDimensions;

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
    FCesiumTextureResource* pTextureResource =
        FCesiumTextureResource::CreateEmpty(
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

    pTexture->SetResource(pTextureResource);

    propertyIt.Value.pTexture = pTexture;
    propertyIt.Value.pResource = pTextureResource;

    ENQUEUE_RENDER_COMMAND(Cesium_InitResource)
    ([pTexture,
      pResource = pTextureResource](FRHICommandListImmediate& RHICmdList) {
      pResource->SetTextureReference(
          pTexture->TextureReference.TextureReferenceRHI);
      pResource->InitResource(FRHICommandListImmediate::Get());
    });
  }
}

FVoxelDataTextures::~FVoxelDataTextures() {}

bool FVoxelDataTextures::canBeDestroyed() const {
  return this->_loadingSlots.size() == 0;
}

UTexture* FVoxelDataTextures::getTexture(const FString& attributeId) const {
  const TextureData* pProperty = this->_propertyMap.Find(attributeId);
  return pProperty ? pProperty->pTexture : nullptr;
}

/**
 * NOTE: This function assumes that the data being read from pData is the same
 * type that the texture expects. Coercive encoding behavior (similar to what
 * is done for CesiumPropertyTableProperty) could be added in the future.
 */
/*static*/ void FVoxelDataTextures::directCopyToTexture(
    const FCesiumPropertyAttributeProperty& property,
    const FVoxelDataTextures::TextureData& data,
    const FUpdateTextureRegion3D& updateRegion) {
  if (!data.pResource || !data.pTexture)
    return;

  const uint8* pData =
      reinterpret_cast<const uint8*>(property.getAccessorData());

  ENQUEUE_RENDER_COMMAND(Cesium_DirectCopyVoxels)
  ([pTexture = data.pTexture,
    pResource = data.pResource,
    format = data.encodedFormat.format,
    &property,
    updateRegion,
    texelSizeBytes = data.texelSizeBytes,
    pData](FRHICommandListImmediate& RHICmdList) {
    if (!IsValid(pTexture)) {
      return;
    }

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

/*static*/ void FVoxelDataTextures::incrementalWriteToTexture(
    const FCesiumPropertyAttributeProperty& property,
    const FVoxelDataTextures::TextureData& data,
    const FUpdateTextureRegion3D& updateRegion) {
  if (!data.pResource || !data.pTexture)
    return;

  ENQUEUE_RENDER_COMMAND(Cesium_IncrementalWriteVoxels)
  ([pTexture = data.pTexture,
    pResource = data.pResource,
    format = data.encodedFormat.format,
    &property,
    updateRegion,
    texelSizeBytes =
        data.texelSizeBytes](FRHICommandListImmediate& RHICmdList) {
    // We're trusting that Cesium3DTileset will destroy its attached
    // CesiumVoxelRendererComponent (and thus the VoxelDataTextures)
    // before unloading glTFs. As long as the texture is valid, so is the
    // CesiumPropertyAttributeProperty.
    if (!IsValid(pTexture)) {
      return;
    }

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

int64 FVoxelDataTextures::add(const UCesiumGltfVoxelComponent& voxelComponent) {
  int64 slotIndex = this->reserveNextSlot();
  if (slotIndex < 0) {
    return -1;
  }

  // Compute the update region for the data textures.
  FUpdateTextureRegion3D updateRegion;
  updateRegion.Width = this->_dataDimensions.x;
  updateRegion.Height = this->_dataDimensions.y;
  updateRegion.Depth = this->_dataDimensions.z;
  updateRegion.SrcX = 0;
  updateRegion.SrcY = 0;
  updateRegion.SrcZ = 0;

  uint32 zSlice = this->_tileCountAlongAxes.x * this->_tileCountAlongAxes.y;
  uint32 indexZ = slotIndex / zSlice;
  uint32 indexY = (slotIndex % zSlice) / this->_tileCountAlongAxes.x;
  uint32 indexX = slotIndex % this->_tileCountAlongAxes.x;
  updateRegion.DestZ = indexZ * this->_dataDimensions.z;
  updateRegion.DestY = indexY * this->_dataDimensions.y;
  updateRegion.DestX = indexX * this->_dataDimensions.x;

  uint32 index = static_cast<uint32>(slotIndex);

  for (auto PropertyIt : this->_propertyMap) {
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

bool FVoxelDataTextures::release(int64_t slotIndex) {
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

int64 FVoxelDataTextures::reserveNextSlot() {
  // Remove head from list of empty slots
  FVoxelDataTextures::Slot* pSlot = this->_pEmptySlotsHead;
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

bool FVoxelDataTextures::isSlotLoaded(int64 index) const {
  if (index < 0 || index >= int64(this->_slots.size()))
    return false;

  return this->_slots[size_t(index)].fence &&
         this->_slots[size_t(index)].fence->IsFenceComplete();
}

bool FVoxelDataTextures::pollLoadingSlots() {
  size_t loadingSlotCount = this->_loadingSlots.size();
  std::erase_if(this->_loadingSlots, [thiz = this](size_t i) {
    return thiz->isSlotLoaded(i);
  });
  return loadingSlotCount != this->_loadingSlots.size();
}
