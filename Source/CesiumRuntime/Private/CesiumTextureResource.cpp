#include "CesiumTextureResource.h"
#include "Stats/Stats.h"

namespace {

ESamplerFilter convertFilter(TextureFilter filter) {
  switch (filter) {
  case TF_Nearest:
    return ESamplerFilter::SF_Point;
  case TF_Bilinear:
    return ESamplerFilter::SF_Bilinear;
  default:
    // case TF_Trilinear:
    // case TF_Default:
    // case TF_MAX:
    return ESamplerFilter::SF_AnisotropicLinear;
  }
}

ESamplerAddressMode convertAddressMode(TextureAddress address) {
  switch (address) {
  case TA_Wrap:
    return ESamplerAddressMode::AM_Wrap;
  case TA_Mirror:
    return ESamplerAddressMode::AM_Mirror;
  default:
    // case TA_Clamp:
    // case TA_MAX:
    return ESamplerAddressMode::AM_Clamp;
  }
}

/**
 * @brief Copies an in-memory glTF mip to the destination. Respects arbitrary
 * row strides at the destination.
 *
 * @param pDest The pre-allocated destination.
 * @param destPitch The row stride in bytes, at the destination. If this is 0,
 * the source mip will be bulk copied.
 * @param format The pixel format.
 * @param src The source image to copy from.
 * @param mipIndex The mip index to copy over.
 */
void CopyMip(
    void* pDest,
    uint32 destPitch,
    EPixelFormat format,
    const CesiumGltf::ImageCesium& src,
    uint32 mipIndex) {
  const CesiumGltf::ImageCesiumMipPosition& mipPos = src.mipPositions[mipIndex];
  uint32 mipWidth =
      FMath::Max<uint32>(static_cast<uint32>(src.width) >> mipIndex, 1);
  uint32 mipHeight =
      FMath::Max<uint32>(static_cast<uint32>(src.height) >> mipIndex, 1);

  const void* pSrcData =
      static_cast<const void*>(&src.pixelData[mipPos.byteOffset]);

  // for platforms that returned 0 pitch from Lock, we need to just use the bulk
  // data directly, never do runtime block size checking, conversion, or the
  // like
  if (destPitch == 0) {
    FMemory::Memcpy(pDest, pSrcData, mipPos.byteSize);
  } else {
    const uint32 blockSizeX =
        GPixelFormats[format].BlockSizeX; // Block width in pixels
    const uint32 blockSizeY =
        GPixelFormats[format].BlockSizeY; // Block height in pixels
    const uint32 blockBytes = GPixelFormats[format].BlockBytes;
    uint32 numColumns =
        (mipWidth + blockSizeX - 1) /
        blockSizeX; // Num-of columns in the source data (in blocks)
    uint32 numRows = (mipHeight + blockSizeY - 1) /
                     blockSizeY; // Num-of rows in the source data (in blocks)
    if (format == PF_PVRTC2 || format == PF_PVRTC4) {
      // PVRTC has minimum 2 blocks width and height
      numColumns = FMath::Max<uint32>(numColumns, 2);
      numRows = FMath::Max<uint32>(numRows, 2);
    }

    const uint32 srcPitch =
        numColumns * blockBytes; // Num-of bytes per row in the source data

    // Copy the texture data.
    CopyTextureData2D(pSrcData, pDest, mipHeight, format, srcPitch, destPitch);
  }
}

} // namespace

FCesiumTextureResourceBase::FCesiumTextureResourceBase(
    TextureGroup textureGroup,
    uint32 width,
    uint32 height,
    EPixelFormat format,
    TextureFilter filter,
    TextureAddress addressX,
    TextureAddress addressY,
    bool sRGB,
    uint32 extData)
    : _textureGroup(textureGroup),
      _width(width),
      _height(height),
      _format(format),
      _filter(convertFilter(filter)),
      _addressX(convertAddressMode(addressX)),
      _addressY(convertAddressMode(addressY)),
      _platformExtData(extData) {
  this->bGreyScaleFormat = (_format == PF_G8) || (_format == PF_BC4);
  this->bSRGB = sRGB;
  STAT(this->_lodGroupStatName = TextureGroupStatFNames[this->_textureGroup]);
}

#if ENGINE_VERSION_5_3_OR_HIGHER
void FCesiumTextureResourceBase::InitRHI(FRHICommandListBase& RHICmdList) {
#else
void FCesiumTextureResourceBase::InitRHI() {
#endif
  FSamplerStateInitializerRHI samplerStateInitializer(
      this->_filter,
      this->_addressX,
      this->_addressY,
      AM_Wrap,
      0.0f,
      0,
      0.0f,
      FLT_MAX);
  this->SamplerStateRHI = GetOrCreateSamplerState(samplerStateInitializer);

  // Create a custom sampler state for using this texture in a deferred pass,
  // where ddx / ddy are discontinuous
  FSamplerStateInitializerRHI deferredSamplerStateInitializer(
      this->_filter,
      this->_addressX,
      this->_addressY,
      AM_Wrap,
      0.0f,
      // Disable anisotropic filtering, since aniso doesn't respect MaxLOD
      1,
      0.0f,
      // Prevent the less detailed mip levels from being used, which hides
      // artifacts on silhouettes due to ddx / ddy being very large This has
      // the side effect that it increases minification aliasing on light
      // functions
      2.0f);
  this->DeferredPassSamplerStateRHI =
      GetOrCreateSamplerState(deferredSamplerStateInitializer);

  this->TextureRHI = this->InitializeTextureRHI();

  RHIUpdateTextureReference(TextureReferenceRHI, this->TextureRHI);

#if STATS
  ETextureCreateFlags textureFlags = TexCreate_ShaderResource;
  if (this->bSRGB) {
    textureFlags |= TexCreate_SRGB;
  }

  const FIntPoint MipExtents =
      CalcMipMapExtent(this->_width, this->_height, this->_format, 0);
  uint32 alignment;
  this->_textureSize = RHICalcTexture2DPlatformSize(
      MipExtents.X,
      MipExtents.Y,
      this->_format,
      this->GetCurrentMipCount(),
      1,
      textureFlags,
      FRHIResourceCreateInfo(this->_platformExtData),
      alignment);

  INC_DWORD_STAT_BY(STAT_TextureMemory, this->_textureSize);
  INC_DWORD_STAT_FNAME_BY(this->_lodGroupStatName, this->_textureSize);
#endif
}

void FCesiumTextureResourceBase::ReleaseRHI() {
  DEC_DWORD_STAT_BY(STAT_TextureMemory, this->_textureSize);
  DEC_DWORD_STAT_FNAME_BY(this->_lodGroupStatName, this->_textureSize);

  RHIUpdateTextureReference(TextureReferenceRHI, nullptr);

  FTextureResource::ReleaseRHI();
}

#if STATS

// This is copied from TextureResource.cpp. Unfortunately we can't use
// FTextureResource::TextureGroupStatFNames, even though it's static and public,
// because, inexplicably, it isn't DLL exported. So instead we duplicate it
// here.
namespace {
DECLARE_STATS_GROUP(
    TEXT("Texture Group"),
    STATGROUP_TextureGroup,
    STATCAT_Advanced);

// Declare the stats for each Texture Group.
#define DECLARETEXTUREGROUPSTAT(Group)                                         \
  DECLARE_MEMORY_STAT(TEXT(#Group), STAT_##Group, STATGROUP_TextureGroup);
FOREACH_ENUM_TEXTUREGROUP(DECLARETEXTUREGROUPSTAT)
#undef DECLARETEXTUREGROUPSTAT
} // namespace

FName FCesiumTextureResourceBase::TextureGroupStatFNames[TEXTUREGROUP_MAX] = {
#define ASSIGNTEXTUREGROUPSTATNAME(Group) GET_STATFNAME(STAT_##Group),
    FOREACH_ENUM_TEXTUREGROUP(ASSIGNTEXTUREGROUPSTATNAME)
#undef ASSIGNTEXTUREGROUPSTATNAME
};

#endif // #if STATS

FCesiumUseExistingTextureResource::FCesiumUseExistingTextureResource(
    FTextureRHIRef existingTexture,
    TextureGroup textureGroup,
    uint32 width,
    uint32 height,
    EPixelFormat format,
    TextureFilter filter,
    TextureAddress addressX,
    TextureAddress addressY,
    bool sRGB,
    uint32 extData)
    : FCesiumTextureResourceBase(
          textureGroup,
          width,
          height,
          format,
          filter,
          addressX,
          addressY,
          sRGB,
          extData),
      _pExistingTexture(nullptr) {
  this->TextureRHI = std::move(existingTexture);
}

FCesiumUseExistingTextureResource::FCesiumUseExistingTextureResource(
    FTextureResource* pExistingTexture,
    TextureGroup textureGroup,
    uint32 width,
    uint32 height,
    EPixelFormat format,
    TextureFilter filter,
    TextureAddress addressX,
    TextureAddress addressY,
    bool sRGB,
    uint32 extData)
    : FCesiumTextureResourceBase(
          textureGroup,
          width,
          height,
          format,
          filter,
          addressX,
          addressY,
          sRGB,
          extData),
      _pExistingTexture(pExistingTexture) {}

FTextureRHIRef FCesiumUseExistingTextureResource::InitializeTextureRHI() {
  if (this->_pExistingTexture) {
    return this->_pExistingTexture->TextureRHI;
  } else {
    return this->TextureRHI;
  }
}

FCesiumCreateNewTextureResource::FCesiumCreateNewTextureResource(
    CesiumGltf::ImageCesium&& image,
    TextureGroup textureGroup,
    uint32 width,
    uint32 height,
    EPixelFormat format,
    TextureFilter filter,
    TextureAddress addressX,
    TextureAddress addressY,
    bool sRGB,
    uint32 extData)
    : FCesiumTextureResourceBase(
          textureGroup,
          width,
          height,
          format,
          filter,
          addressX,
          addressY,
          sRGB,
          extData),
      _image(std::move(image)) {}

FTextureRHIRef FCesiumCreateNewTextureResource::InitializeTextureRHI() {
  FRHIResourceCreateInfo createInfo{TEXT("CesiumTextureUtility")};
  createInfo.BulkData = nullptr;
  createInfo.ExtData = _platformExtData;

  ETextureCreateFlags textureFlags = TexCreate_ShaderResource;

  if (this->bSRGB) {
    textureFlags |= TexCreate_SRGB;
  }

  uint32 mipCount =
      FMath::Max(1, static_cast<int32>(this->_image.mipPositions.size()));

  // Create a new RHI texture, initially empty.

  // RHICreateTexture2D can actually copy over all the mips in one shot,
  // but it expects a particular memory layout. Might be worth configuring
  // Cesium Native's mip-map generation to obey a standard memory layout.
  FTexture2DRHIRef rhiTexture =
      RHICreateTexture(FRHITextureCreateDesc::Create2D(createInfo.DebugName)
                           .SetExtent(int32(this->_width), int32(this->_height))
                           .SetFormat(this->_format)
                           .SetNumMips(uint8(mipCount))
                           .SetNumSamples(1)
                           .SetFlags(textureFlags)
                           .SetInitialState(ERHIAccess::Unknown)
                           .SetExtData(createInfo.ExtData)
                           .SetGPUMask(createInfo.GPUMask)
                           .SetClearValue(createInfo.ClearValueBinding));

  // Copy over all image data (including mip levels)
  for (uint32 i = 0; i < mipCount; ++i) {
    uint32 DestPitch;
    void* pDestination =
        RHILockTexture2D(rhiTexture, i, RLM_WriteOnly, DestPitch, false);
    CopyMip(pDestination, DestPitch, _format, this->_image, i);
    RHIUnlockTexture2D(rhiTexture, i, false);
  }

  return rhiTexture;
}
