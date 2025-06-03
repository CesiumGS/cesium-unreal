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

/**
 * A Cesium texture resource that creates an initially empty `FRHITexture` for
 * FVoxelDataTextures.
 */
class FCesiumVoxelDataTextureResource : public FCesiumTextureResource {
public:
  FCesiumVoxelDataTextureResource(
      TextureGroup textureGroup,
      uint32 width,
      uint32 height,
      uint32 depth,
      EPixelFormat format,
      TextureFilter filter,
      TextureAddress addressX,
      TextureAddress addressY,
      bool sRGB,
      uint32 extData);

protected:
  virtual FTextureRHIRef InitializeTextureRHI() override;
};

FCesiumVoxelDataTextureResource::FCesiumVoxelDataTextureResource(
    TextureGroup textureGroup,
    uint32 width,
    uint32 height,
    uint32 depth,
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
          depth,
          format,
          filter,
          addressX,
          addressY,
          sRGB,
          false,
          extData,
          true) {}

FTextureRHIRef FCesiumVoxelDataTextureResource::InitializeTextureRHI() {
  FRHIResourceCreateInfo createInfo{TEXT("FCesiumVoxelDataTextureResource")};
  createInfo.BulkData = nullptr;
  createInfo.ExtData = this->_platformExtData;

  ETextureCreateFlags textureFlags = TexCreate_ShaderResource;
  if (this->bSRGB) {
    textureFlags |= TexCreate_SRGB;
  }

  // Create a new 3D RHI texture, initially empty.
  return RHICreateTexture(
      FRHITextureCreateDesc::Create3D(createInfo.DebugName)
          .SetExtent(int32(this->_width), int32(this->_height))
          .SetDepth(this->_depth)
          .SetFormat(this->_format)
          .SetNumMips(1)
          .SetNumSamples(1)
          .SetFlags(textureFlags)
          .SetInitialState(ERHIAccess::Unknown)
          .SetExtData(createInfo.ExtData)
          .SetGPUMask(createInfo.GPUMask)
          .SetClearValue(createInfo.ClearValueBinding));
}

FVoxelDataTextures::FVoxelDataTextures(
    const FCesiumVoxelClassDescription* pVoxelClass,
    const glm::uvec3& dataDimensions,
    ERHIFeatureLevel::Type featureLevel,
    uint32 requestedMemoryPerTexture)
    : _slots(),
      _pEmptySlotsHead(nullptr),
      _pOccupiedSlotsHead(nullptr),
      _dataDimensions(dataDimensions),
      _tileCountAlongAxes(0),
      _maximumTileCount(0),
      _propertyMap() {
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
    this->_propertyMap.Add(Property.Name, {encodedFormat, nullptr, nullptr});
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
    pSlot->Index = static_cast<int64>(i);
    pSlot->Previous = i > 0 ? &_slots[i - 1] : nullptr;
    pSlot->Next = i < _slots.size() - 1 ? &_slots[i + 1] : nullptr;
  }

  this->_pEmptySlotsHead = &this->_slots[0];

  // Create the actual textures.
  for (auto& propertyIt : this->_propertyMap) {
    FCesiumTextureResource* pTextureResource =
        MakeUnique<FCesiumVoxelDataTextureResource>(
            TextureGroup::TEXTUREGROUP_8BitData,
            actualDimensions.x,
            actualDimensions.y,
            actualDimensions.z,
            propertyIt.Value.encodedFormat.format,
            TextureFilter::TF_Nearest,
            TextureAddress::TA_Clamp,
            TextureAddress::TA_Clamp,
            false,
            0)
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

FVoxelDataTextures ::~FVoxelDataTextures() {
  for (auto propertyIt : this->_propertyMap) {
    UTexture* pTexture = propertyIt.Value.pTexture;
    propertyIt.Value.pTexture = nullptr;
    propertyIt.Value.pResource = nullptr;

    if (IsValid(pTexture)) {
      pTexture->RemoveFromRoot();
      CesiumLifetime::destroy(pTexture);
    }
  }
}

/**
 * NOTE: This function assumes that the data being read from pData is the same
 * type that the texture expects. Coercive encoding behavior (similar to what
 * is done for CesiumPropertyTableProperty) could be added in the future.
 */
static void writeTo3DTexture(
    UTexture* pTexture,
    FCesiumTextureResource* pResource,
    const std::byte* pData,
    FUpdateTextureRegion3D updateRegion,
    uint32 texelSizeBytes) {
  if (!pResource || !pData)
    return;

  ENQUEUE_RENDER_COMMAND(Cesium_CopyVoxels)
  ([pTexture, pResource, pData, updateRegion, texelSizeBytes](
       FRHICommandListImmediate& RHICmdList) {
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
        (const uint8*)(pData));
  });
}

int64 FVoxelDataTextures::Add(const UCesiumGltfVoxelComponent& voxelComponent) {
  uint32 slotIndex = ReserveNextSlot();
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
    const ValidatedVoxelBuffer* pValidBuffer =
        voxelComponent.attributeBuffers.Find(PropertyIt.Key);
    if (!pValidBuffer) {
      continue;
    }

    uint32 texelSizeBytes = PropertyIt.Value.encodedFormat.bytesPerChannel *
                            PropertyIt.Value.encodedFormat.channels;
    const std::byte* pData = pValidBuffer->pBuffer->cesium.data.data();
    pData += pValidBuffer->pBufferView->byteOffset;

    writeTo3DTexture(
        PropertyIt.Value.pTexture,
        PropertyIt.Value.pResource,
        pData,
        updateRegion,
        texelSizeBytes);
  }
  return slotIndex;
}

int64 FVoxelDataTextures::ReserveNextSlot() {
  // Remove head from list of empty slots
  FVoxelDataTextures::Slot* pSlot = this->_pEmptySlotsHead;
  if (!pSlot) {
    return -1;
  }

  this->_pEmptySlotsHead = pSlot->Next;

  if (this->_pEmptySlotsHead) {
    this->_pEmptySlotsHead->Previous = nullptr;
  }

  // Move to list of occupied slots (as the new head)
  pSlot->Next = this->_pOccupiedSlotsHead;
  if (pSlot->Next) {
    this->_pOccupiedSlotsHead->Previous = pSlot;
  }
  this->_pOccupiedSlotsHead = pSlot;

  return pSlot->Index;
}

bool FVoxelDataTextures::Release(uint32 slotIndex) {
  if (slotIndex >= this->_slots.size()) {
    return false; // Index out of bounds
  }

  Slot* pSlot = &this->_slots[slotIndex];
  if (pSlot->Previous) {
    pSlot->Previous->Next = pSlot->Next;
  }
  if (pSlot->Next) {
    pSlot->Next->Previous = pSlot->Previous;
  }

  // Move to list of empty slots (as the new head)
  pSlot->Next = this->_pEmptySlotsHead;
  if (pSlot->Next) {
    pSlot->Next->Previous = pSlot;
  }

  pSlot->Previous = nullptr;
  this->_pEmptySlotsHead = pSlot;

  return true;
}
