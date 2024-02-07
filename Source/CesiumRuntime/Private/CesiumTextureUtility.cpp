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

using namespace CesiumGltf;

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

struct ExtensionUnrealTexture {
  static inline constexpr const char* TypeName = "ExtensionUnrealTexture";
  static inline constexpr const char* ExtensionName = "PRIVATE_unreal_texture";

  CesiumUtility::IntrusivePointer<
      CesiumTextureUtility::ReferenceCountedUnrealTexture>
      pTexture;
};

} // namespace

namespace CesiumTextureUtility {

ReferenceCountedUnrealTexture::ReferenceCountedUnrealTexture(
    TObjectPtr<UTexture2D> p) noexcept
    : pTexture(p) {
  if (this->pTexture) {
    this->pTexture->AddToRoot();
  }
}

ReferenceCountedUnrealTexture::~ReferenceCountedUnrealTexture() noexcept {
  UTexture2D* pLocal = this->pTexture;
  this->pTexture = nullptr;

  if (IsValid(pLocal)) {
    AsyncTask(ENamedThreads::GameThread, [pLocal]() {
      pLocal->RemoveFromRoot();
      CesiumLifetime::destroy(pLocal);
    });
  }
}

TUniquePtr<LoadedTextureResult> loadTextureFromModelAnyThreadPart(
    CesiumGltf::Model& model,
    const CesiumGltf::Texture& texture,
    bool sRGB,
    std::vector<FCesiumTextureResourceBase*>& textureResources) {
  check(textureResources.size() == model.images.size());

  int64_t textureIndex =
      model.textures.empty() ? -1 : &texture - &model.textures[0];
  if (textureIndex < 0 || size_t(textureIndex) >= model.textures.size()) {
    textureIndex = -1;
  }

  const ExtensionUnrealTexture* pUnrealTextureExtension =
      texture.getExtension<ExtensionUnrealTexture>();
  if (pUnrealTextureExtension && pUnrealTextureExtension->pTexture) {
    // There's an existing Unreal texture for this glTF texture.
    // This will commonly be the case when this model was upsampled from a
    // parent tile.
    TUniquePtr<LoadedTextureResult> pResult = MakeUnique<LoadedTextureResult>();
    pResult->pTexture = pUnrealTextureExtension->pTexture;
    pResult->textureIndex = textureIndex;
    return pResult;
  }

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

  CesiumGltf::Image& image = model.images[source];
  const CesiumGltf::ImageCesium& imageCesium = image.cesium;
  const CesiumGltf::Sampler& sampler =
      model.getSafe(model.samplers, texture.sampler);

  FCesiumTextureResourceBase* pExistingImageResource = nullptr;

  if (image.cesium.pixelData.empty() && source >= 0 &&
      source < textureResources.size()) {
    // An RHI texture has already been created for this image; reuse it.
    pExistingImageResource = textureResources[source];
  }

  TUniquePtr<LoadedTextureResult> pResult =
      loadTextureFromImageAndSamplerAnyThreadPart(
          image,
          sampler,
          sRGB,
          pExistingImageResource);
  if (pResult) {
    // Note the index of this texture within the glTF.
    pResult->textureIndex = textureIndex;

    if (source >= 0 && source < textureResources.size()) {
      // Make the RHI resource known so it can be used by other textures that
      // reference this same image.
      textureResources[source] = pResult->pTextureResource.Get();
    }
  }
  return pResult;
}

TUniquePtr<LoadedTextureResult> loadTextureFromImageAndSamplerAnyThreadPart(
    CesiumGltf::Image& image,
    const CesiumGltf::Sampler& sampler,
    bool sRGB,
    FCesiumTextureResourceBase* pExistingImageResource) {
  // glTF spec: "When undefined, a sampler with repeat wrapping and auto
  // filtering should be used."
  TextureAddress addressX = TextureAddress::TA_Wrap;
  TextureAddress addressY = TextureAddress::TA_Wrap;

  TextureFilter filter = TextureFilter::TF_Default;
  bool useMipMapsIfAvailable = false;

  switch (sampler.wrapS) {
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

  switch (sampler.wrapT) {
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

  if (sampler.magFilter && !sampler.minFilter) {
    // Only a magnification filter is specified, so use it.
    filter =
        sampler.magFilter.value() == CesiumGltf::Sampler::MagFilter::NEAREST
            ? TextureFilter::TF_Nearest
            : TextureFilter::TF_Default;
  } else if (sampler.minFilter) {
    // Use specified minFilter.
    switch (sampler.minFilter.value()) {
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

  switch (sampler.minFilter.value_or(
      CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_LINEAR)) {
  case CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_LINEAR:
  case CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_NEAREST:
  case CesiumGltf::Sampler::MinFilter::NEAREST_MIPMAP_LINEAR:
  case CesiumGltf::Sampler::MinFilter::NEAREST_MIPMAP_NEAREST:
    useMipMapsIfAvailable = true;
    break;
  default: // LINEAR and NEAREST
    useMipMapsIfAvailable = false;
    break;
  }

  return loadTextureAnyThreadPart(
      image.cesium,
      addressX,
      addressY,
      filter,
      useMipMapsIfAvailable,
      // TODO: allow texture group to be configured on Cesium3DTileset.
      TEXTUREGROUP_World,
      sRGB,
      std::nullopt,
      pExistingImageResource);
}

static UTexture2D* CreateTexture2D(LoadedTextureResult* pHalfLoadedTexture) {
  if (!pHalfLoadedTexture) {
    return nullptr;
  }

  UTexture2D* pTexture = pHalfLoadedTexture->pTexture
                             ? pHalfLoadedTexture->pTexture->pTexture
                             : nullptr;
  if (!pTexture) {
    pTexture = NewObject<UTexture2D>(
        GetTransientPackage(),
        MakeUniqueObjectName(
            GetTransientPackage(),
            UTexture2D::StaticClass(),
            "CesiumRuntimeTexture"),
        RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);

    pTexture->AddressX = pHalfLoadedTexture->addressX;
    pTexture->AddressY = pHalfLoadedTexture->addressY;
    pTexture->Filter = pHalfLoadedTexture->filter;
    pTexture->LODGroup = pHalfLoadedTexture->group;
    pTexture->SRGB = pHalfLoadedTexture->sRGB;

    pTexture->NeverStream = true;

    pHalfLoadedTexture->pTexture = new ReferenceCountedUnrealTexture(pTexture);
  }

  return pTexture;
}

TUniquePtr<LoadedTextureResult> loadTextureAnyThreadPart(
    CesiumGltf::ImageCesium& imageCesium,
    TextureAddress addressX,
    TextureAddress addressY,
    TextureFilter filter,
    bool useMipMapsIfAvailable,
    TextureGroup group,
    bool sRGB,
    std::optional<EPixelFormat> overridePixelFormat,
    FCesiumTextureResourceBase* pExistingImageResource) {
  EPixelFormat pixelFormat;
  if (imageCesium.compressedPixelFormat != GpuCompressedPixelFormat::NONE) {
    switch (imageCesium.compressedPixelFormat) {
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
  } else if (overridePixelFormat) {
    pixelFormat = *overridePixelFormat;
  } else {
    switch (imageCesium.channels) {
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

  pResult->addressX = addressX;
  pResult->addressY = addressY;
  pResult->filter = filter;
  pResult->group = group;
  pResult->sRGB = sRGB;

  // Store the current size of the pixel data, because we're about to clear it
  // but we still want to have an accurate estimation of the size of the image
  // for caching purposes.
  imageCesium.sizeBytes = int64_t(imageCesium.pixelData.size());

  if (pExistingImageResource) {
    pResult->pTextureResource = MakeUnique<FCesiumUseExistingTextureResource>(
        pExistingImageResource,
        group,
        imageCesium.width,
        imageCesium.height,
        pixelFormat,
        filter,
        addressX,
        addressY,
        sRGB,
        0);
  } else if (
      GRHISupportsAsyncTextureCreation && !imageCesium.pixelData.empty()) {
    // Create RHI texture resource on this worker thread, and then hand it off
    // to the renderer thread.
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::CreateRHITexture2D)

    FTexture2DRHIRef textureReference =
        CreateRHITexture2D_Async(imageCesium, pixelFormat, sRGB);
    pResult->pTextureResource = MakeUnique<FCesiumUseExistingTextureResource>(
        textureReference,
        group,
        imageCesium.width,
        imageCesium.height,
        pixelFormat,
        filter,
        addressX,
        addressY,
        sRGB,
        // TODO: "ExtData" (whatever that is) usually comes from
        // UTexture2D::GetPlatformData()->GetExtData(). But we don't have a
        // UTexture2D yet. Do we really need it?
        0);

    // Clear the now-unnecessary copy of the pixel data. Calling clear() isn't
    // good enough because it won't actually release the memory.
    std::vector<std::byte> pixelData;
    imageCesium.pixelData.swap(pixelData);

    std::vector<CesiumGltf::ImageCesiumMipPosition> mipPositions;
    imageCesium.mipPositions.swap(mipPositions);
  } else {
    // The RHI texture will be created later on the render thread, directly
    // from this texture source. We need valid pixelData here, though.
    if (imageCesium.pixelData.empty()) {
      return nullptr;
    }

    pResult->pTextureResource = MakeUnique<FCesiumCreateNewTextureResource>(
        std::move(imageCesium),
        group,
        imageCesium.width,
        imageCesium.height,
        pixelFormat,
        filter,
        addressX,
        addressY,
        sRGB,
        0);
  }

  check(pResult->pTextureResource != nullptr);

  return pResult;
}

CesiumUtility::IntrusivePointer<ReferenceCountedUnrealTexture>
loadTextureGameThreadPart(
    CesiumGltf::Model& model,
    LoadedTextureResult* pHalfLoadedTexture) {
  if (pHalfLoadedTexture == nullptr)
    return nullptr;

  CesiumUtility::IntrusivePointer<ReferenceCountedUnrealTexture> pResult =
      loadTextureGameThreadPart(pHalfLoadedTexture);

  if (pResult && pHalfLoadedTexture && pHalfLoadedTexture->textureIndex >= 0 &&
      size_t(pHalfLoadedTexture->textureIndex) < model.textures.size()) {
    CesiumGltf::Texture& texture =
        model.textures[pHalfLoadedTexture->textureIndex];
    ExtensionUnrealTexture& extension =
        texture.addExtension<ExtensionUnrealTexture>();
    extension.pTexture = pHalfLoadedTexture->pTexture;
  }

  return pHalfLoadedTexture->pTexture;
}

CesiumUtility::IntrusivePointer<ReferenceCountedUnrealTexture>
loadTextureGameThreadPart(LoadedTextureResult* pHalfLoadedTexture) {
  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::LoadTexture)

  UTexture2D* pTexture = CreateTexture2D(pHalfLoadedTexture);
  if (pTexture == nullptr) {
    return nullptr;
  }

  FCesiumTextureResourceBase* pCesiumTextureResource =
      pHalfLoadedTexture->pTextureResource.Release();
  if (pCesiumTextureResource) {
    pTexture->SetResource(pCesiumTextureResource);

    ENQUEUE_RENDER_COMMAND(Cesium_InitResource)
    ([pTexture, pCesiumTextureResource](FRHICommandListImmediate& RHICmdList) {
      pCesiumTextureResource->SetTextureReference(
          pTexture->TextureReference.TextureReferenceRHI);
#if ENGINE_VERSION_5_3_OR_HIGHER
      pCesiumTextureResource->InitResource(
          FRHICommandListImmediate::Get()); // Init Resource now requires a
                                            // command list.
#else
      pCesiumTextureResource->InitResource();
#endif
    });
  }

  return pHalfLoadedTexture->pTexture;
}

void destroyHalfLoadedTexture(LoadedTextureResult& halfLoaded) {
  if (halfLoaded.pTextureResource) {
    ENQUEUE_RENDER_COMMAND(Cesium_ReleaseHalfLoadedTexture)
    ([pTextureResource = std::move(halfLoaded.pTextureResource)](
         FRHICommandListImmediate& RHICmdList) mutable {
      pTextureResource->TextureRHI.SafeRelease();
    });
  }
}

void destroyTexture(UTexture* pTexture) {
  check(pTexture != nullptr);
  CesiumLifetime::destroy(pTexture);
}
} // namespace CesiumTextureUtility
