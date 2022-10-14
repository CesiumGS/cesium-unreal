// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumTextureUtility.h"
#include "Async/Async.h"
#include "Async/Future.h"
#include "Async/TaskGraphInterfaces.h"
#include "CesiumLifetime.h"
#include "CesiumRuntime.h"
#include "Containers/ResourceArray.h"
#include "DynamicRHI.h"
#include "GenericPlatform/GenericPlatformProcess.h"
#include "PixelFormat.h"
#include "RHIDefinitions.h"
#include "RHIResources.h"
#include "RenderUtils.h"
#include "Runtime/Launch/Resources/Version.h"
#include "TextureResource.h"
#include <CesiumGltf/ExtensionKhrTextureBasisu.h>
#include <CesiumGltf/ExtensionTextureWebp.h>
#include <CesiumGltf/ImageCesium.h>
#include <CesiumGltf/Ktx2TranscodeTargets.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumUtility/Tracing.h>
#include <memory>
#include <stb_image_resize.h>
#include <variant>

#define LEGACY_TEXTURE_CREATION 0

using namespace CesiumGltf;

namespace {
#if LEGACY_TEXTURE_CREATION
// Legacy texture creation code path. Only for testing, no safety checks are
// done.

void legacy_populateMips(
    FTexturePlatformData& platformData,
    const CesiumGltf::ImageCesium& image,
    bool generateMipMaps) {
  uint32 width = static_cast<uint32>(image.width);
  uint32 height = static_cast<uint32>(image.height);
  for (const CesiumGltf::ImageCesiumMipPosition& mipPos : image.mipPositions) {
    FTexture2DMipMap* pMip = new FTexture2DMipMap();
    platformData.Mips.Add(pMip);
    pMip->SizeX = width;
    pMip->SizeY = height;

    pMip->BulkData.Lock(LOCK_READ_WRITE);
    void* pDest = pMip->BulkData.Realloc(static_cast<int64>(mipPos.byteSize));

    FMemory::Memcpy(
        pDest,
        &image.pixelData[mipPos.byteOffset],
        mipPos.byteSize);

    pMip->BulkData.Unlock();

    if (!generateMipMaps) {
      // Only populate mip 0 if we don't want the full mip chain.
      return;
    }

    if (width > 1) {
      width >>= 1;
    }

    if (height > 1) {
      height >>= 1;
    }
  }
}
#endif

struct GetImageFromSource {
  CesiumGltf::ImageCesium*
  operator()(CesiumTextureUtility::GltfImagePtr& imagePtr) {
    return imagePtr.pImage;
  }

  CesiumGltf::ImageCesium*
  operator()(CesiumTextureUtility::EmbeddedImageSource& embeddedImage) {
    return &embeddedImage.image;
  }

  template <typename TSource>
  CesiumGltf::ImageCesium* operator()(TSource& /*source*/) {
    return nullptr;
  }
};

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

/**
 * @brief A wrapper for CesiumGltf::ImageCesium that allows it to be used as
 * bulk data for RHI texture creation. This prevents having to copy texture
 * data to an intermediary, Unreal-specific CPU buffer before being copied to
 * the GPU (in particular FTexture2DMipMap).
 */
class FCesiumTextureData : public FResourceBulkDataInterface {
public:
  FCesiumTextureData(const CesiumGltf::ImageCesium& image)
      : _textureData(image.pixelData.data()),
        _textureDataSize(
            (!image.mipPositions.empty())
                ? static_cast<uint32>(image.mipPositions[0].byteSize)
                : static_cast<uint32>(image.pixelData.size())) {}

  const void* GetResourceBulkData() const override {
    return static_cast<const void*>(this->_textureData);
  }

  uint32 GetResourceBulkDataSize() const override {
    return this->_textureDataSize;
  }

  void Discard() override {}

private:
  const std::byte* _textureData;
  uint32 _textureDataSize;
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
 * @brief An RHI resource that creates and destroys RHI textures. If a non-null
 * FTexture2DRHIRef is given, ownership of it is assumed and it will be
 * destroyed in ReleaseResource. Otherwise, the RHI texture will be created
 * from the in-memory Cesium glTF image in InitRHI (on the render thread).
 *
 */
class FCesiumTextureResource : public FTextureResource {
public:
  FCesiumTextureResource(
      UTexture* pTexture,
      CesiumTextureUtility::CesiumTextureSource&& textureSource,
      uint32 width,
      uint32 height,
      EPixelFormat format,
      TextureFilter filter,
      TextureAddress addressX,
      TextureAddress addressY,
      bool sRGB,
      uint32 extData)
      : _pTexture(pTexture),
        _textureSource(std::move(textureSource)),
        _width(width),
        _height(height),
        _format(format),
        _filter(convertFilter(filter)),
        _addressX(convertAddressMode(addressX)),
        _addressY(convertAddressMode(addressY)),
        _platformExtData(extData) {
    this->bGreyScaleFormat = (_format == PF_G8) || (_format == PF_BC4);
    this->bSRGB = sRGB;

    CesiumTextureUtility::AsyncCreatedTexture* pAsyncTexture =
        std::get_if<CesiumTextureUtility::AsyncCreatedTexture>(
            &this->_textureSource);
    if (pAsyncTexture) {
      this->TextureRHI = pAsyncTexture->rhiTextureRef;
      pAsyncTexture->rhiTextureRef.SafeRelease();
    }
  }

  virtual ~FCesiumTextureResource() {
    check(this->_pTexture != nullptr);
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 27
    this->_pTexture->Resource = nullptr;
#else
    this->_pTexture->SetResource(nullptr);
#endif
  }

  uint32 GetSizeX() const override { return this->_width; }

  uint32 GetSizeY() const override { return this->_height; }

  virtual void InitRHI() override {
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

    if (!this->TextureRHI) {
      // Asynchronous RHI texture creation was not available. So create it now
      // directly from the in-memory cesium mips.
      CesiumGltf::ImageCesium* pImage =
          std::visit(GetImageFromSource{}, this->_textureSource);

      check(pImage != nullptr);

      // Wrap mip0 as a bulk data source.
      FCesiumTextureData bulkData(*pImage);

      FRHIResourceCreateInfo createInfo{};
      createInfo.BulkData = &bulkData;
      createInfo.ExtData = _platformExtData;

      ETextureCreateFlags textureFlags = TexCreate_ShaderResource;

      if (this->bSRGB) {
        textureFlags |= TexCreate_SRGB;
      }

      uint32 mipCount =
          FMath::Max(1, static_cast<int32>(pImage->mipPositions.size()));

      FTexture2DRHIRef rhiTexture;

      // Copies over mip0, allocates the rest of the mips if needed.

      // RHICreateTexture2D can actually copy over all the mips in one shot,
      // but it expects a particular memory layout. Might be worth configuring
      // Cesium Native's mip-map generation to obey a standard memory layout.
      rhiTexture = RHICreateTexture2D(
          this->_width,
          this->_height,
          this->_format,
          mipCount,
          1,
          textureFlags,
          createInfo);

      // Copies over rest of the mips
      for (uint32 i = 1; i < mipCount; ++i) {
        uint32 DestPitch;
        void* pDestination =
            RHILockTexture2D(rhiTexture, i, RLM_WriteOnly, DestPitch, false);
        CopyMip(pDestination, DestPitch, _format, *pImage, i);
        RHIUnlockTexture2D(rhiTexture, i, false);
      }

      this->TextureRHI = std::move(rhiTexture);
    }

    RHIUpdateTextureReference(TextureReferenceRHI, this->TextureRHI);
  }

  virtual void ReleaseRHI() override {
    RHIUpdateTextureReference(TextureReferenceRHI, nullptr);

    FTextureResource::ReleaseRHI();
  }

private:
  UTexture* _pTexture;
  CesiumTextureUtility::CesiumTextureSource _textureSource;

  uint32 _width;
  uint32 _height;
  EPixelFormat _format;
  ESamplerFilter _filter;
  ESamplerAddressMode _addressX;
  ESamplerAddressMode _addressY;

  uint32 _platformExtData;
};

/**
 * @brief Create an RHI texture on this thread. This requires
 * GRHISupportsAsyncTextureCreation to be true.
 *
 * @param image The CPU image to create on the GPU.
 * @param format The pixel format of the image.
 * @param generateMipMaps Whether the RHI texture should have a mipmap.
 * @param Whether to use a sRGB color-space.
 * @return The RHI texture reference.
 */
FTexture2DRHIRef CreateRHITexture2D_Async(
    const CesiumGltf::ImageCesium& image,
    EPixelFormat format,
    bool generateMipMaps,
    bool sRGB) {
  check(GRHISupportsAsyncTextureCreation);

  ETextureCreateFlags textureFlags = TexCreate_ShaderResource;
  if (sRGB) {
    textureFlags |= TexCreate_SRGB;
  }

  if (generateMipMaps) {
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

    return RHIAsyncCreateTexture2D(
        static_cast<uint32>(image.width),
        static_cast<uint32>(image.height),
        format,
        mipCount,
        textureFlags,
        mipsData,
        mipCount);
  } else {
    void* pTextureData = (void*)(image.pixelData.data());
    return RHIAsyncCreateTexture2D(
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

namespace CesiumTextureUtility {

GltfImagePtr
GltfImageIndex::resolveImage(const CesiumGltf::Model& model) const {
  // Almost certainly a developer error otherwise.
  assert(this->imageIndex >= 0 && this->imageIndex < model.images.size());

  // It's not worth making const-specializations of CesiumTextureSource, so we
  // just const-cast.
  CesiumGltf::ImageCesium* pImage = const_cast<CesiumGltf::ImageCesium*>(
      &model.images[static_cast<size_t>(this->index)].cesium);

  return GltfImagePtr{pImage};
}

TUniquePtr<FTexturePlatformData>
createTexturePlatformData(int32 sizeX, int32 sizeY, EPixelFormat format) {
  if (sizeX > 0 && sizeY > 0 &&
      (sizeX % GPixelFormats[format].BlockSizeX) == 0 &&
      (sizeY % GPixelFormats[format].BlockSizeY) == 0) {
    TUniquePtr<FTexturePlatformData> pTexturePlatformData =
        MakeUnique<FTexturePlatformData>();
    pTexturePlatformData->SizeX = sizeX;
    pTexturePlatformData->SizeY = sizeY;
    pTexturePlatformData->PixelFormat = format;

    return pTexturePlatformData;
  } else {
    return nullptr;
  }
}

static UTexture2D* CreateTexture2D(LoadedTextureResult* pHalfLoadedTexture) {
  if (!pHalfLoadedTexture) {
    return nullptr;
  }

  UTexture2D* pTexture = pHalfLoadedTexture->pTexture.Get();
  if (!pTexture && pHalfLoadedTexture->pTextureData) {
    pTexture = NewObject<UTexture2D>(
        GetTransientPackage(),
        MakeUniqueObjectName(
            GetTransientPackage(),
            UTexture2D::StaticClass(),
            "CesiumRuntimeTexture"),
        RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);

#if ENGINE_MAJOR_VERSION >= 5
    pTexture->SetPlatformData(pHalfLoadedTexture->pTextureData.Release());
#else
    pTexture->PlatformData = pHalfLoadedTexture->pTextureData.Release();
#endif
    pTexture->AddressX = pHalfLoadedTexture->addressX;
    pTexture->AddressY = pHalfLoadedTexture->addressY;
    pTexture->Filter = pHalfLoadedTexture->filter;
    pTexture->LODGroup = pHalfLoadedTexture->group;
    pTexture->SRGB = pHalfLoadedTexture->sRGB;

    pTexture->NeverStream = true;

    pHalfLoadedTexture->pTexture = pTexture;
  }

  return pTexture;
}

TUniquePtr<LoadedTextureResult> loadTextureAnyThreadPart(
    CesiumTextureSource&& imageSource,
    const TextureAddress& addressX,
    const TextureAddress& addressY,
    const TextureFilter& filter,
    const TextureGroup& group,
    bool generateMipMaps,
    bool sRGB) {

  CesiumGltf::ImageCesium* pImage =
      std::visit(GetImageFromSource{}, imageSource);

  assert(pImage != nullptr);
  CesiumGltf::ImageCesium& image = *pImage;

  if (image.pixelData.empty() || image.width == 0 || image.height == 0) {
    return nullptr;
  }

  if (generateMipMaps) {
    std::optional<std::string> errorMessage =
        CesiumGltfReader::GltfReader::generateMipMaps(image);
    if (errorMessage) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT("%s"),
          UTF8_TO_TCHAR(errorMessage->c_str()));
    }
  }

  EPixelFormat pixelFormat;
  if (image.compressedPixelFormat != GpuCompressedPixelFormat::NONE) {
    switch (image.compressedPixelFormat) {
    case GpuCompressedPixelFormat::ETC1_RGB:
      pixelFormat = EPixelFormat::PF_ETC1;
      break;
    case GpuCompressedPixelFormat::ETC2_RGBA:
      pixelFormat = EPixelFormat::PF_ETC2_RGBA;
      break;
    case GpuCompressedPixelFormat::BC1_RGB:
      pixelFormat = EPixelFormat::PF_DXT1;
      break;
    case GpuCompressedPixelFormat::BC3_RGBA:
      pixelFormat = EPixelFormat::PF_DXT5;
      break;
    case GpuCompressedPixelFormat::BC4_R:
      pixelFormat = EPixelFormat::PF_BC4;
      break;
    case GpuCompressedPixelFormat::BC5_RG:
      pixelFormat = EPixelFormat::PF_BC5;
      break;
    case GpuCompressedPixelFormat::BC7_RGBA:
      pixelFormat = EPixelFormat::PF_BC7;
      break;
    case GpuCompressedPixelFormat::ASTC_4x4_RGBA:
      pixelFormat = EPixelFormat::PF_ASTC_4x4;
      break;
    case GpuCompressedPixelFormat::PVRTC2_4_RGBA:
      pixelFormat = EPixelFormat::PF_PVRTC2;
      break;
    case GpuCompressedPixelFormat::ETC2_EAC_R11:
      pixelFormat = EPixelFormat::PF_ETC2_R11_EAC;
      break;
    case GpuCompressedPixelFormat::ETC2_EAC_RG11:
      pixelFormat = EPixelFormat::PF_ETC2_RG11_EAC;
      break;
    default:
      // Unsupported compressed texture format.
      return nullptr;
    };
  } else {
    switch (image.channels) {
    case 1:
      pixelFormat = PF_R8;
      break;
    case 2:
      pixelFormat = PF_R8G8;
      break;
    case 3:
    case 4:
    default:
      pixelFormat = PF_R8G8B8A8;
    };
  }

  TUniquePtr<LoadedTextureResult> pResult = MakeUnique<LoadedTextureResult>();
  pResult->pTextureData =
      createTexturePlatformData(image.width, image.height, pixelFormat);

  if (!pResult->pTextureData) {
    return nullptr;
  }

  pResult->addressX = addressX;
  pResult->addressY = addressY;
  pResult->filter = filter;
  pResult->group = group;
  pResult->sRGB = sRGB;
  pResult->generateMipMaps = generateMipMaps;

#if LEGACY_TEXTURE_CREATION
  // Legacy texture creation copies mip data into the FTexturePlatformData.
  legacy_populateMips(*pResult->pTextureData, image, generateMipMaps);
  // Mark the image source as legacy, so we later know where to look for image
  // data.
  pResult->textureSource = LegacyTextureSource{};
#else
  if (GRHISupportsAsyncTextureCreation) {
    // Create RHI texture resource asynchronously.
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::CreateRHITexture2D)

    pResult->textureSource = AsyncCreatedTexture{
        CreateRHITexture2D_Async(image, pixelFormat, generateMipMaps, sRGB)};
  } else {
    // The RHI texture will be created later on the render thread, directly
    // from this texture source.
    pResult->textureSource = std::move(imageSource);
  }
#endif

  return pResult;
}

TUniquePtr<LoadedTextureResult> loadTextureAnyThreadPart(
    CesiumGltf::Model& model,
    const CesiumGltf::Texture& texture,
    bool sRGB) {

  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::LoadTexture)

  const CesiumGltf::ExtensionKhrTextureBasisu* pKtxExtension =
      texture.getExtension<CesiumGltf::ExtensionKhrTextureBasisu>();
  const CesiumGltf::ExtensionTextureWebp* pWebpExtension =
      texture.getExtension<CesiumGltf::ExtensionTextureWebp>();

  int32_t source = -1;
  if (pKtxExtension) {
    if (pKtxExtension->source < 0 ||
        pKtxExtension->source >= model.images.size()) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT(
              "KTX texture source index must be non-negative and less than %d, but is %d"),
          model.images.size(),
          pKtxExtension->source);
      return nullptr;
    }
    source = pKtxExtension->source;
  } else if (pWebpExtension) {
    if (pWebpExtension->source < 0 ||
        pWebpExtension->source >= model.images.size()) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT(
              "WebP texture source index must be non-negative and less than %d, but is %d"),
          model.images.size(),
          pWebpExtension->source);
      return nullptr;
    }
    source = pWebpExtension->source;
  } else {
    if (texture.source < 0 || texture.source >= model.images.size()) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT(
              "Texture source index must be non-negative and less than %d, but is %d"),
          model.images.size(),
          texture.source);
      return nullptr;
    }
    source = texture.source;
  }

  CesiumGltf::ImageCesium& image = model.images[source].cesium;
  const CesiumGltf::Sampler* pSampler =
      CesiumGltf::Model::getSafe(&model.samplers, texture.sampler);

  // glTF spec: "When undefined, a sampler with repeat wrapping and auto
  // filtering should be used."
  TextureAddress addressX = TextureAddress::TA_Wrap;
  TextureAddress addressY = TextureAddress::TA_Wrap;

  TextureFilter filter = TextureFilter::TF_Default;
  bool useMipMaps = false;

  if (pSampler) {
    switch (pSampler->wrapS) {
    case CesiumGltf::Sampler::WrapS::CLAMP_TO_EDGE:
      addressX = TextureAddress::TA_Clamp;
      break;
    case CesiumGltf::Sampler::WrapS::MIRRORED_REPEAT:
      addressX = TextureAddress::TA_Mirror;
      break;
    case CesiumGltf::Sampler::WrapS::REPEAT:
      addressX = TextureAddress::TA_Wrap;
      break;
    }

    switch (pSampler->wrapT) {
    case CesiumGltf::Sampler::WrapT::CLAMP_TO_EDGE:
      addressY = TextureAddress::TA_Clamp;
      break;
    case CesiumGltf::Sampler::WrapT::MIRRORED_REPEAT:
      addressY = TextureAddress::TA_Mirror;
      break;
    case CesiumGltf::Sampler::WrapT::REPEAT:
      addressY = TextureAddress::TA_Wrap;
      break;
    }

    // Unreal Engine's available filtering modes are only nearest, bilinear,
    // trilinear, and "default". Default means "use the texture group settings",
    // and the texture group settings are defined in a config file and can
    // vary per platform. All filter modes can use mipmaps if they're available,
    // but only TF_Default will ever use anisotropic texture filtering.
    //
    // Unreal also doesn't separate the minification filter from the
    // magnification filter. So we'll just ignore the magFilter unless it's the
    // only filter specified.
    //
    // Generally our bias is toward TF_Default, because that gives the user more
    // control via texture groups.

    if (pSampler->magFilter && !pSampler->minFilter) {
      // Only a magnification filter is specified, so use it.
      filter =
          pSampler->magFilter.value() == CesiumGltf::Sampler::MagFilter::NEAREST
              ? TextureFilter::TF_Nearest
              : TextureFilter::TF_Default;
    } else if (pSampler->minFilter) {
      // Use specified minFilter.
      switch (pSampler->minFilter.value()) {
      case CesiumGltf::Sampler::MinFilter::NEAREST:
      case CesiumGltf::Sampler::MinFilter::NEAREST_MIPMAP_NEAREST:
        filter = TextureFilter::TF_Nearest;
        break;
      case CesiumGltf::Sampler::MinFilter::LINEAR:
      case CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_NEAREST:
        filter = TextureFilter::TF_Bilinear;
        break;
      default:
        filter = TextureFilter::TF_Default;
        break;
      }
    } else {
      // No filtering specified at all, let the texture group decide.
      filter = TextureFilter::TF_Default;
    }

    switch (pSampler->minFilter.value_or(
        CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_LINEAR)) {
    case CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_LINEAR:
    case CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_NEAREST:
    case CesiumGltf::Sampler::MinFilter::NEAREST_MIPMAP_LINEAR:
    case CesiumGltf::Sampler::MinFilter::NEAREST_MIPMAP_NEAREST:
      useMipMaps = true;
      break;
    default: // LINEAR and NEAREST
      useMipMaps = false;
      break;
    }
  }

  TUniquePtr<LoadedTextureResult> result = loadTextureAnyThreadPart(
      GltfImagePtr{&image},
      addressX,
      addressY,
      filter,
      TextureGroup::TEXTUREGROUP_World,
      useMipMaps,
      sRGB);

  // Replace the image pointer with an index, in case the pointer gets
  // invalidated before the main thread loading continues.
  if (result && std::get_if<GltfImagePtr>(&result->textureSource)) {
    result->textureSource = GltfImageIndex{source};
  }

  return result;
}

UTexture2D* loadTextureGameThreadPart(LoadedTextureResult* pHalfLoadedTexture) {
  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::LoadTexture)

  if (!pHalfLoadedTexture) {
    return nullptr;
  }

  if (pHalfLoadedTexture->pTexture.Get()) {
    return pHalfLoadedTexture->pTexture.Get();
  }

  UTexture2D* pTexture = CreateTexture2D(pHalfLoadedTexture);

  if (std::get_if<LegacyTextureSource>(&pHalfLoadedTexture->textureSource)) {
    pTexture->UpdateResource();
    return pTexture;
  }

  FCesiumTextureResource* pCesiumTextureResource = new FCesiumTextureResource(
      pTexture,
      std::move(pHalfLoadedTexture->textureSource),
      static_cast<uint32>(pTexture->GetSizeX()),
      static_cast<uint32>(pTexture->GetSizeY()),
      pTexture->GetPixelFormat(),
      pHalfLoadedTexture->filter,
      pHalfLoadedTexture->addressX,
      pHalfLoadedTexture->addressY,
      pHalfLoadedTexture->sRGB,
      pTexture->PlatformData->GetExtData());

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 27
  pTexture->Resource = pCesiumTextureResource;
#else
  pTexture->SetResource(pCesiumTextureResource);
#endif

  ENQUEUE_RENDER_COMMAND(Cesium_InitResource)
  ([pTexture, pCesiumTextureResource](FRHICommandListImmediate& RHICmdList) {
    pCesiumTextureResource->SetTextureReference(
        pTexture->TextureReference.TextureReferenceRHI);
    pCesiumTextureResource->InitResource();
  });

  return pTexture;
}

UTexture2D* loadTextureGameThreadPart(
    const CesiumGltf::Model& model,
    LoadedTextureResult* pHalfLoadedTexture) {
  if (!pHalfLoadedTexture) {
    return nullptr;
  }

  GltfImageIndex* pImageIndex =
      std::get_if<GltfImageIndex>(&pHalfLoadedTexture->textureSource);
  if (pImageIndex) {
    pHalfLoadedTexture->textureSource = pImageIndex->resolveImage(model);
  }

  return loadTextureGameThreadPart(pHalfLoadedTexture);
}

void destroyHalfLoadedTexture(LoadedTextureResult& halfLoaded) {
  AsyncCreatedTexture* pAsyncCreatedTexture =
      std::get_if<AsyncCreatedTexture>(&halfLoaded.textureSource);
  if (pAsyncCreatedTexture) {
    // An RHI texture was asynchronously created and must now be destroyed.
    ENQUEUE_RENDER_COMMAND(Cesium_ReleaseHalfLoadedTexture)
    ([rhiTextureRef = pAsyncCreatedTexture->rhiTextureRef](
         FRHICommandListImmediate& RHICmdList) mutable {
      rhiTextureRef.SafeRelease();
    });

    pAsyncCreatedTexture->rhiTextureRef.SafeRelease();
  }
}

void destroyTexture(UTexture* pTexture) {
  check(pTexture != nullptr);
  CesiumLifetime::destroy(pTexture);
}
} // namespace CesiumTextureUtility
