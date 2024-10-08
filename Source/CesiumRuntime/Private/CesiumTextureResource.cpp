// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumTextureResource.h"
#include "CesiumRuntime.h"
#include "CesiumTextureUtility.h"
#include "Misc/CoreStats.h"
#include "RenderUtils.h"
#include <CesiumGltfReader/GltfReader.h>

namespace {

/**
 * A Cesium texture resource that uses an already-created `FRHITexture`. This is
 * used when `GRHISupportsAsyncTextureCreation` is true and so we were already
 * able to create the FRHITexture in a worker thread. It is also used when a
 * single glTF `Image` is referenced by multiple glTF `Texture` instances. We
 * only need one `FRHITexture` is this case, but we need multiple
 * `FTextureResource` instances to support the different sampler settings that
 * are likely used in the different textures.
 */
class FCesiumUseExistingTextureResource : public FCesiumTextureResource {
public:
  FCesiumUseExistingTextureResource(
      FTextureRHIRef existingTexture,
      TextureGroup textureGroup,
      uint32 width,
      uint32 height,
      EPixelFormat format,
      TextureFilter filter,
      TextureAddress addressX,
      TextureAddress addressY,
      bool sRGB,
      bool useMipsIfAvailable,
      uint32 extData,
      bool isPrimary);

  FCesiumUseExistingTextureResource(
      const TSharedPtr<FTextureResource>& pExistingTexture,
      TextureGroup textureGroup,
      uint32 width,
      uint32 height,
      EPixelFormat format,
      TextureFilter filter,
      TextureAddress addressX,
      TextureAddress addressY,
      bool sRGB,
      bool useMipsIfAvailable,
      uint32 extData,
      bool isPrimary);

protected:
  virtual FTextureRHIRef InitializeTextureRHI() override;

private:
  TSharedPtr<FTextureResource> _pExistingTexture;
};

/**
 * A Cesium texture resource that creates an `FRHITexture` from a glTF
 * `ImageCesium` when `InitRHI` is called from the render thread. When
 * `GRHISupportsAsyncTextureCreation` is false (everywhere but Direct3D), we can
 * only create a `FRHITexture` on the render thread, so this is the code that
 * does it.
 */
class FCesiumCreateNewTextureResource : public FCesiumTextureResource {
public:
  FCesiumCreateNewTextureResource(
      CesiumGltf::ImageCesium&& image,
      TextureGroup textureGroup,
      uint32 width,
      uint32 height,
      EPixelFormat format,
      TextureFilter filter,
      TextureAddress addressX,
      TextureAddress addressY,
      bool sRGB,
      bool useMipsIfAvailable,
      uint32 extData);

protected:
  virtual FTextureRHIRef InitializeTextureRHI() override;

private:
  CesiumGltf::ImageCesium _image;
};

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
  size_t byteOffset = 0;
  size_t byteSize = 0;
  if (src.mipPositions.empty()) {
    byteOffset = 0;
    byteSize = src.pixelData.size();
  } else {
    const CesiumGltf::ImageCesiumMipPosition& mipPos =
        src.mipPositions[mipIndex];
    byteOffset = mipPos.byteOffset;
    byteSize = mipPos.byteSize;
  }
  uint32 mipWidth =
      FMath::Max<uint32>(static_cast<uint32>(src.width) >> mipIndex, 1);
  uint32 mipHeight =
      FMath::Max<uint32>(static_cast<uint32>(src.height) >> mipIndex, 1);

  const void* pSrcData = static_cast<const void*>(&src.pixelData[byteOffset]);

  // for platforms that returned 0 pitch from Lock, we need to just use the bulk
  // data directly, never do runtime block size checking, conversion, or the
  // like
  if (destPitch == 0) {
    FMemory::Memcpy(pDest, pSrcData, byteSize);
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

FTexture2DRHIRef createAsyncTextureAndWait(
    uint32 SizeX,
    uint32 SizeY,
    uint8 Format,
    uint32 NumMips,
    ETextureCreateFlags Flags,
    void** InitialMipData,
    uint32 NumInitialMips) {

#if ENGINE_VERSION_5_4_OR_HIGHER
  FGraphEventRef CompletionEvent;

  FTexture2DRHIRef result = RHIAsyncCreateTexture2D(
      SizeX,
      SizeY,
      Format,
      NumMips,
      Flags,
      ERHIAccess::Unknown,
      InitialMipData,
      NumInitialMips,
      TEXT("CesiumTexture"),
      CompletionEvent);

  if (CompletionEvent) {
    CompletionEvent->Wait();
  }

  return result;
#elif ENGINE_VERSION_5_3_OR_HIGHER
  FGraphEventRef CompletionEvent;

  FTexture2DRHIRef result = RHIAsyncCreateTexture2D(
      SizeX,
      SizeY,
      Format,
      NumMips,
      Flags,
      InitialMipData,
      NumInitialMips,
      CompletionEvent);

  if (CompletionEvent) {
    CompletionEvent->Wait();
  }

  return result;
#else
  return RHIAsyncCreateTexture2D(
      SizeX,
      SizeY,
      Format,
      NumMips,
      Flags,
      InitialMipData,
      NumInitialMips);
#endif
}

/**
 * @brief Create an RHI texture on this thread. This requires
 * GRHISupportsAsyncTextureCreation to be true.
 *
 * @param image The CPU image to create on the GPU.
 * @param format The pixel format of the image.
 * @param Whether to use a sRGB color-space.
 * @return The RHI texture reference.
 */
FTexture2DRHIRef CreateRHITexture2D_Async(
    const CesiumGltf::ImageCesium& image,
    EPixelFormat format,
    bool sRGB) {
  check(GRHISupportsAsyncTextureCreation);

  ETextureCreateFlags textureFlags = TexCreate_ShaderResource;

  // Just like in FCesiumCreateNewTextureResource, we're assuming here that we
  // can create an FRHITexture as sRGB, and later create another
  // UTexture2D / FTextureResource pointing to the same FRHITexture that is not
  // sRGB (or vice-versa), and that Unreal will effectively ignore the flag on
  // FRHITexture.
  if (sRGB) {
    textureFlags |= TexCreate_SRGB;
  }

  if (!image.mipPositions.empty()) {
    // Here 16 is a generously large (but arbitrary) hard limit for number of
    // mips.
    uint32 mipCount = static_cast<uint32>(image.mipPositions.size());
    if (mipCount > 16) {
      mipCount = 16;
    }

    void* mipsData[16];
    for (size_t i = 0; i < mipCount; ++i) {
      const CesiumGltf::ImageCesiumMipPosition& mipPos = image.mipPositions[i];
      mipsData[i] = (void*)(&image.pixelData[mipPos.byteOffset]);
    }

    return createAsyncTextureAndWait(
        static_cast<uint32>(image.width),
        static_cast<uint32>(image.height),
        format,
        mipCount,
        textureFlags,
        mipsData,
        mipCount);
  } else {
    void* pTextureData = (void*)(image.pixelData.data());
    return createAsyncTextureAndWait(
        static_cast<uint32>(image.width),
        static_cast<uint32>(image.height),
        format,
        1,
        textureFlags,
        &pTextureData,
        1);
  }
}

} // namespace

void FCesiumTextureResourceDeleter::operator()(FCesiumTextureResource* p) {
  FCesiumTextureResource::Destroy(p);
}

/*static*/ FCesiumTextureResourceUniquePtr FCesiumTextureResource::CreateNew(
    CesiumGltf::ImageCesium& imageCesium,
    TextureGroup textureGroup,
    const std::optional<EPixelFormat>& overridePixelFormat,
    TextureFilter filter,
    TextureAddress addressX,
    TextureAddress addressY,
    bool sRGB,
    bool needsMipMaps) {
  if (imageCesium.pixelData.empty()) {
    return nullptr;
  }

  if (needsMipMaps) {
    std::optional<std::string> errorMessage =
        CesiumGltfReader::GltfReader::generateMipMaps(imageCesium);
    if (errorMessage) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT("%s"),
          UTF8_TO_TCHAR(errorMessage->c_str()));
    }
  }

  std::optional<EPixelFormat> maybePixelFormat =
      CesiumTextureUtility::getPixelFormatForImageCesium(
          imageCesium,
          overridePixelFormat);
  if (!maybePixelFormat) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "Image cannot be created because it has an unsupported compressed pixel format (%d)."),
        imageCesium.compressedPixelFormat);
    return nullptr;
  }

  // Store the current size of the pixel data, because
  // we're about to clear it but we still want to have
  // an accurate estimation of the size of the image for
  // caching purposes.
  imageCesium.sizeBytes = int64_t(imageCesium.pixelData.size());

  if (GRHISupportsAsyncTextureCreation) {
    // Create RHI texture resource on this worker
    // thread, and then hand it off to the renderer
    // thread.
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::CreateRHITexture2D)

    FTexture2DRHIRef textureReference =
        CreateRHITexture2D_Async(imageCesium, *maybePixelFormat, sRGB);
    textureReference->SetName(
        FName(UTF8_TO_TCHAR(imageCesium.getUniqueAssetId().c_str())));
    auto pResult = TUniquePtr<
        FCesiumUseExistingTextureResource,
        FCesiumTextureResourceDeleter>(new FCesiumUseExistingTextureResource(
        textureReference,
        textureGroup,
        imageCesium.width,
        imageCesium.height,
        *maybePixelFormat,
        filter,
        addressX,
        addressY,
        sRGB,
        needsMipMaps,
        0,
        true));

    // Clear the now-unnecessary copy of the pixel data.
    // Calling clear() isn't good enough because it
    // won't actually release the memory.
    std::vector<std::byte> pixelData;
    imageCesium.pixelData.swap(pixelData);

    std::vector<CesiumGltf::ImageCesiumMipPosition> mipPositions;
    imageCesium.mipPositions.swap(mipPositions);

    return pResult;
  } else {
    // The RHI texture will be created later on the
    // render thread, directly from this texture source.
    // We need valid pixelData here, though.
    auto pResult = TUniquePtr<
        FCesiumCreateNewTextureResource,
        FCesiumTextureResourceDeleter>(new FCesiumCreateNewTextureResource(
        std::move(imageCesium),
        textureGroup,
        imageCesium.width,
        imageCesium.height,
        *maybePixelFormat,
        filter,
        addressX,
        addressY,
        sRGB,
        needsMipMaps,
        0));
    return pResult;
  }
}

FCesiumTextureResourceUniquePtr FCesiumTextureResource::CreateWrapped(
    const TSharedPtr<FCesiumTextureResource>& pExistingResource,
    TextureGroup textureGroup,
    TextureFilter filter,
    TextureAddress addressX,
    TextureAddress addressY,
    bool sRGB,
    bool useMipMapsIfAvailable) {
  if (pExistingResource == nullptr)
    return nullptr;

  return FCesiumTextureResourceUniquePtr(new FCesiumUseExistingTextureResource(
      pExistingResource,
      textureGroup,
      pExistingResource->_width,
      pExistingResource->_height,
      pExistingResource->_format,
      filter,
      addressX,
      addressY,
      sRGB,
      useMipMapsIfAvailable,
      0,
      false));
}

/*static*/ void FCesiumTextureResource::Destroy(FCesiumTextureResource* p) {
  if (p == nullptr)
    return;

  ENQUEUE_RENDER_COMMAND(DeleteResource)
  ([p](FRHICommandListImmediate& RHICmdList) {
    p->ReleaseResource();
    delete p;
  });
}

FCesiumTextureResource::FCesiumTextureResource(
    TextureGroup textureGroup,
    uint32 width,
    uint32 height,
    EPixelFormat format,
    TextureFilter filter,
    TextureAddress addressX,
    TextureAddress addressY,
    bool sRGB,
    bool useMipsIfAvailable,
    uint32 extData,
    bool isPrimary)
    : _textureGroup(textureGroup),
      _width(width),
      _height(height),
      _format(format),
      _filter(convertFilter(filter)),
      _addressX(convertAddressMode(addressX)),
      _addressY(convertAddressMode(addressY)),
      _useMipsIfAvailable(useMipsIfAvailable),
      _platformExtData(extData),
      _isPrimary(isPrimary) {
  this->bGreyScaleFormat = (_format == PF_G8) || (_format == PF_BC4);
  this->bSRGB = sRGB;
  STAT(this->_lodGroupStatName = TextureGroupStatFNames[this->_textureGroup]);
}

#if ENGINE_VERSION_5_3_OR_HIGHER
void FCesiumTextureResource::InitRHI(FRHICommandListBase& RHICmdList) {
#else
void FCesiumTextureResource::InitRHI() {
#endif
  FSamplerStateInitializerRHI samplerStateInitializer(
      this->_filter,
      this->_addressX,
      this->_addressY,
      AM_Wrap,
      0.0f,
      0,
      0.0f,
      this->_useMipsIfAvailable ? FLT_MAX : 1.0f);
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
      // artifacts on silhouettes due to ddx / ddy being very large. This has
      // the side effect that it increases minification aliasing on light
      // functions
      this->_useMipsIfAvailable ? 2.0f : 1.0f);
  this->DeferredPassSamplerStateRHI =
      GetOrCreateSamplerState(deferredSamplerStateInitializer);

  this->TextureRHI = this->InitializeTextureRHI();

  RHIUpdateTextureReference(TextureReferenceRHI, this->TextureRHI);

#if STATS
  if (this->_isPrimary) {
    ETextureCreateFlags textureFlags = TexCreate_ShaderResource;
    if (this->bSRGB) {
      textureFlags |= TexCreate_SRGB;
    }

    const FIntPoint MipExtents =
        CalcMipMapExtent(this->_width, this->_height, this->_format, 0);
    const FRHIResourceCreateInfo CreateInfo(this->_platformExtData);

    FDynamicRHI::FRHICalcTextureSizeResult result = RHICalcTexturePlatformSize(
        FRHITextureDesc::Create2D(
            MipExtents,
            this->_format,
            CreateInfo.ClearValueBinding,
            textureFlags,
            this->GetCurrentMipCount(),
            1,
            CreateInfo.ExtData),
        0);

    this->_textureSize = result.Size;

    INC_DWORD_STAT_BY(STAT_TextureMemory, this->_textureSize);
    INC_DWORD_STAT_FNAME_BY(this->_lodGroupStatName, this->_textureSize);
  }
#endif
}

void FCesiumTextureResource::ReleaseRHI() {
#if STATS
  if (this->_isPrimary) {
    DEC_DWORD_STAT_BY(STAT_TextureMemory, this->_textureSize);
    DEC_DWORD_STAT_FNAME_BY(this->_lodGroupStatName, this->_textureSize);
  }
#endif

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

FName FCesiumTextureResource::TextureGroupStatFNames[TEXTUREGROUP_MAX] = {
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
    bool useMipsIfAvailable,
    uint32 extData,
    bool isPrimary)
    : FCesiumTextureResource(
          textureGroup,
          width,
          height,
          format,
          filter,
          addressX,
          addressY,
          sRGB,
          useMipsIfAvailable,
          extData,
          isPrimary),
      _pExistingTexture(nullptr) {
  this->TextureRHI = std::move(existingTexture);
}

FCesiumUseExistingTextureResource::FCesiumUseExistingTextureResource(
    const TSharedPtr<FTextureResource>& pExistingTexture,
    TextureGroup textureGroup,
    uint32 width,
    uint32 height,
    EPixelFormat format,
    TextureFilter filter,
    TextureAddress addressX,
    TextureAddress addressY,
    bool sRGB,
    bool useMipsIfAvailable,
    uint32 extData,
    bool isPrimary)
    : FCesiumTextureResource(
          textureGroup,
          width,
          height,
          format,
          filter,
          addressX,
          addressY,
          sRGB,
          useMipsIfAvailable,
          extData,
          isPrimary),
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
    bool useMipsIfAvailable,
    uint32 extData)
    : FCesiumTextureResource(
          textureGroup,
          width,
          height,
          format,
          filter,
          addressX,
          addressY,
          sRGB,
          useMipsIfAvailable,
          extData,
          true),
      _image(std::move(image)) {}

FTextureRHIRef FCesiumCreateNewTextureResource::InitializeTextureRHI() {
  // Use the asset ID as the name of the texture so it will be visible in the
  // Render Resource Viewer.
  FString debugName = TEXT("CesiumTextureUtility");
  if (!this->_image.getUniqueAssetId().empty()) {
    debugName = UTF8_TO_TCHAR(this->_image.getUniqueAssetId().c_str());
  }

  FRHIResourceCreateInfo createInfo{*debugName};
  createInfo.BulkData = nullptr;
  createInfo.ExtData = _platformExtData;

  ETextureCreateFlags textureFlags = TexCreate_ShaderResource;

  // What if a texture is treated as sRGB in one context but not another?
  // In glTF, whether or not a texture should be treated as sRGB depends on how
  // it's _used_. A texture used for baseColorFactor or emissiveFactor should be
  // sRGB, while all others should be linear. It's unlikely - but not impossible
  // - for a single glTF Texture or Image to be used in one context where it
  // must be sRGB, and another where it must be linear. Unreal also has an sRGB
  // flag on FTextureResource and on UTexture2D (neither of which are shared),
  // so _hopefully_ those will apply even if the underlying FRHITexture (which
  // is shared) says differently. If not, we'll likely end up treating the
  // second texture incorrectly. Confirming an answer here will be time
  // consuming, and the scenario is quite unlikely, so we're strategically
  // leaving this an open question.
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

  // Clear the now-unnecessary copy of the pixel data. Calling clear() isn't
  // good enough because it won't actually release the memory.
  std::vector<std::byte> pixelData;
  this->_image.pixelData.swap(pixelData);

  std::vector<CesiumGltf::ImageCesiumMipPosition> mipPositions;
  this->_image.mipPositions.swap(mipPositions);

  return rhiTexture;
}
