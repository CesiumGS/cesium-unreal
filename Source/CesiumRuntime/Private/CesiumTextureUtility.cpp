// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumTextureUtility.h"
#include "Async/Async.h"
#include "Async/Future.h"
#include "Async/TaskGraphInterfaces.h"
#include "CesiumCommon.h"
#include "CesiumLifetime.h"
#include "CesiumRuntime.h"
#include "CesiumTextureResource.h"
#include "Containers/ResourceArray.h"
#include "DynamicRHI.h"
#include "ExtensionUnrealImageData.h"
#include "GenericPlatform/GenericPlatformProcess.h"
#include "PixelFormat.h"
#include "RHICommandList.h"
#include "RHIDefinitions.h"
#include "RHIResources.h"
#include "RenderUtils.h"
#include "RenderingThread.h"
#include "Runtime/Launch/Resources/Version.h"
#include "TextureResource.h"
#include "UObject/Package.h"
#include <CesiumGltf/ExtensionKhrTextureBasisu.h>
#include <CesiumGltf/ExtensionTextureWebp.h>
#include <CesiumGltf/ImageCesium.h>
#include <CesiumGltf/Ktx2TranscodeTargets.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/ReferenceCountedThreadSafe.h>
#include <CesiumUtility/Tracing.h>
#include <memory>
#include <stb_image_resize.h>
#include <variant>

using namespace CesiumGltf;

namespace {
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

namespace {

FTexture2DRHIRef createAsyncTextureAndWait(
    uint32 SizeX,
    uint32 SizeY,
    uint8 Format,
    uint32 NumMips,
    ETextureCreateFlags Flags,
    void** InitialMipData,
    uint32 NumInitialMips) {
#if ENGINE_VERSION_5_3_OR_HIGHER
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

} // namespace

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

CesiumGltf::ImageCesium& getImageCesium(CesiumGltf::Image& image) {
  ExtensionUnrealImageData* pExtension =
      image.getExtension<ExtensionUnrealImageData>();
  if (pExtension && pExtension->pImage) {
    return pExtension->pImage->image;
  } else {
    return image.cesium;
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
  CesiumGltf::ImageCesium* pImage =
      &getImageCesium(const_cast<CesiumGltf::Image&>(
          model.images[static_cast<size_t>(this->index)]));

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

    pTexture->SetPlatformData(pHalfLoadedTexture->pTextureData.Release());
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
    bool sRGB,
    int32_t textureIndex) {

  CesiumGltf::ImageCesium* pImage =
      std::visit(GetImageFromSource{}, imageSource);

  assert(pImage != nullptr);
  CesiumGltf::ImageCesium& image = *pImage;

  if (generateMipMaps && !image.pixelData.empty()) {
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

  if (GRHISupportsAsyncTextureCreation && !image.pixelData.empty()) {
    // Create RHI texture resource on this worker thread, and then hand it off
    // to the renderer thread.
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::CreateRHITexture2D)

    FTexture2DRHIRef textureReference =
        CreateRHITexture2D_Async(image, pixelFormat, generateMipMaps, sRGB);
    pResult->pTextureResource = MakeUnique<FCesiumUseExistingTextureResource>(
        textureReference,
        group,
        image.width,
        image.height,
        pixelFormat,
        filter,
        addressX,
        addressY,
        sRGB,
        // TODO: "ExtData" (whatever that is) usually comes from
        // UTexture2D::GetPlatformData()->GetExtData(). But we don't have a
        // UTexture2D yet. Do we really need it?
        0);
  } else {
    // The RHI texture will be created later on the render thread, directly
    // from this texture source.
    pResult->pTextureResource = MakeUnique<FCesiumCreateNewTextureResource>(
        std::move(image),
        group,
        image.width,
        image.height,
        pixelFormat,
        filter,
        addressX,
        addressY,
        sRGB,
        0);
  }

  pResult->textureIndex = textureIndex;

  return pResult;
}

TUniquePtr<LoadedTextureResult> loadTextureAnyThreadPart(
    CesiumGltf::Model& model,
    const CesiumGltf::Texture& texture,
    bool sRGB,
    int32_t textureIndex) {

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

  CesiumGltf::ImageCesium& image = getImageCesium(model.images[source]);
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

  // Transfer ownership of the image to the LoadedTextureResult.
  TUniquePtr<LoadedTextureResult> result = loadTextureAnyThreadPart(
      EmbeddedImageSource{std::move(image)},
      addressX,
      addressY,
      filter,
      TextureGroup::TEXTUREGROUP_World,
      useMipMaps,
      sRGB,
      textureIndex);

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
  if (pTexture == nullptr) {
    return nullptr;
  }

  FCesiumTextureResourceBase* pCesiumTextureResource =
      pHalfLoadedTexture->pTextureResource.Release();

  pTexture->SetResource(pCesiumTextureResource);

  ENQUEUE_RENDER_COMMAND(Cesium_InitResource)
  ([pTexture, pCesiumTextureResource](FRHICommandListImmediate& RHICmdList) {
    pCesiumTextureResource->SetTextureReference(
        pTexture->TextureReference.TextureReferenceRHI);
    pCesiumTextureResource->InitResource();
  });

  return pTexture;
}

namespace {

// Associate an Unreal UTexture2D with a glTF Texture.
struct ExtensionUnrealTexture {
  static inline constexpr const char* TypeName = "ExtensionUnrealTexture";
  static inline constexpr const char* ExtensionName = "PRIVATE_unreal_texture";

  TWeakObjectPtr<UTexture2D> pTexture;
};

} // namespace

UTexture2D* loadTextureGameThreadPart(
    CesiumGltf::Model& model,
    LoadedTextureResult* pHalfLoadedTexture) {
  if (!pHalfLoadedTexture) {
    return nullptr;
  }

  GltfImageIndex* pImageIndex =
      std::get_if<GltfImageIndex>(&pHalfLoadedTexture->textureSource);
  if (pImageIndex) {
    pHalfLoadedTexture->textureSource = pImageIndex->resolveImage(model);
  }

  Image* pImage = nullptr;

  if (pHalfLoadedTexture->textureIndex >= 0 &&
      pHalfLoadedTexture->textureIndex < model.textures.size()) {
    // If a UTexture2D already exists for this glTF texture, no need to create
    // one again. In fact, it might not be possible, because the image data may
    // have already been unloaded from memory.
    const Texture& texture = model.textures[pHalfLoadedTexture->textureIndex];
    const ExtensionUnrealTexture* pExtension =
        texture.getExtension<ExtensionUnrealTexture>();
    if (pExtension && pExtension->pTexture.IsValid()) {
      return pExtension->pTexture.Get();
    }

    // It's also possible that the texture refers to an Image for which we have
    // already created a UTexture2D based on a different Texture instance. In
    // that case, too, the glTF pixelData has already been cleared. So we'll
    // create this new texture with a GPU copy from the old textrure.
    pImage = model.getSafe(&model.images, texture.source);
    if (pImage) {
      pExtension = pImage->getExtension<ExtensionUnrealTexture>();
      if (pExtension && pExtension->pTexture.IsValid()) {
        pHalfLoadedTexture->textureSource = pExtension->pTexture;
      }
    }
  }

  UTexture2D* pResult = loadTextureGameThreadPart(pHalfLoadedTexture);

  if (pHalfLoadedTexture->textureIndex >= 0) {
    Texture& texture = model.textures[pHalfLoadedTexture->textureIndex];
    texture.addExtension<ExtensionUnrealTexture>().pTexture = pResult;

    if (pImage) {
      pImage->addExtension<ExtensionUnrealTexture>().pTexture = pResult;
    }
  }

  return pResult;
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
