// Copyright 2020-2024 CesiumGS, Inc. and Contributors

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

struct ExtensionUnrealTexture {
  static inline constexpr const char* TypeName = "ExtensionUnrealTexture";
  static inline constexpr const char* ExtensionName = "PRIVATE_unreal_texture";

  CesiumUtility::IntrusivePointer<
      CesiumTextureUtility::ReferenceCountedUnrealTexture>
      pTexture = nullptr;
};

std::optional<EPixelFormat> getPixelFormatForImageCesium(
    const ImageCesium& imageCesium,
    const std::optional<EPixelFormat> overridePixelFormat) {
  if (imageCesium.compressedPixelFormat != GpuCompressedPixelFormat::NONE) {
    switch (imageCesium.compressedPixelFormat) {
    case GpuCompressedPixelFormat::ETC1_RGB:
      return EPixelFormat::PF_ETC1;
      break;
    case GpuCompressedPixelFormat::ETC2_RGBA:
      return EPixelFormat::PF_ETC2_RGBA;
      break;
    case GpuCompressedPixelFormat::BC1_RGB:
      return EPixelFormat::PF_DXT1;
      break;
    case GpuCompressedPixelFormat::BC3_RGBA:
      return EPixelFormat::PF_DXT5;
      break;
    case GpuCompressedPixelFormat::BC4_R:
      return EPixelFormat::PF_BC4;
      break;
    case GpuCompressedPixelFormat::BC5_RG:
      return EPixelFormat::PF_BC5;
      break;
    case GpuCompressedPixelFormat::BC7_RGBA:
      return EPixelFormat::PF_BC7;
      break;
    case GpuCompressedPixelFormat::ASTC_4x4_RGBA:
      return EPixelFormat::PF_ASTC_4x4;
      break;
    case GpuCompressedPixelFormat::PVRTC2_4_RGBA:
      return EPixelFormat::PF_PVRTC2;
      break;
    case GpuCompressedPixelFormat::ETC2_EAC_R11:
      return EPixelFormat::PF_ETC2_R11_EAC;
      break;
    case GpuCompressedPixelFormat::ETC2_EAC_RG11:
      return EPixelFormat::PF_ETC2_RG11_EAC;
      break;
    default:
      // Unsupported compressed texture format.
      return std::nullopt;
    };
  } else if (overridePixelFormat) {
    return *overridePixelFormat;
  } else {
    switch (imageCesium.channels) {
    case 1:
      return PF_R8;
      break;
    case 2:
      return PF_R8G8;
      break;
    case 3:
    case 4:
    default:
      return PF_R8G8B8A8;
    };
  }

  return std::nullopt;
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

namespace CesiumTextureUtility {

ReferenceCountedUnrealTexture::ReferenceCountedUnrealTexture() noexcept
    : _pUnrealTexture(nullptr), _pTextureResource(nullptr) {}

ReferenceCountedUnrealTexture::~ReferenceCountedUnrealTexture() noexcept {
  UTexture2D* pLocal = this->_pUnrealTexture;
  this->_pUnrealTexture = nullptr;

  if (IsValid(pLocal)) {
    if (IsInGameThread()) {
      pLocal->RemoveFromRoot();
      CesiumLifetime::destroy(pLocal);
    } else {
      AsyncTask(ENamedThreads::GameThread, [pLocal]() {
        pLocal->RemoveFromRoot();
        CesiumLifetime::destroy(pLocal);
      });
    }
  }
}

TObjectPtr<UTexture2D> ReferenceCountedUnrealTexture::getUnrealTexture() const {
  return this->_pUnrealTexture;
}

void ReferenceCountedUnrealTexture::setUnrealTexture(
    const TObjectPtr<UTexture2D>& p) {
  if (p == this->_pUnrealTexture)
    return;

  if (p) {
    p->AddToRoot();
  }

  if (this->_pUnrealTexture) {
    this->_pUnrealTexture->RemoveFromRoot();
  }

  this->_pUnrealTexture = p;
}

const TUniquePtr<FCesiumTextureResourceBase>&
ReferenceCountedUnrealTexture::getTextureResource() const {
  return this->_pTextureResource;
}

TUniquePtr<FCesiumTextureResourceBase>&
ReferenceCountedUnrealTexture::getTextureResource() {
  return this->_pTextureResource;
}

void ReferenceCountedUnrealTexture::setTextureResource(
    TUniquePtr<FCesiumTextureResourceBase>&& p) {
  this->_pTextureResource = std::move(p);
}

CesiumGltf::SharedAsset<ImageCesium>
ReferenceCountedUnrealTexture::getSharedImage() const {
  return this->_pImageCesium;
}

void ReferenceCountedUnrealTexture::setSharedImage(
    CesiumGltf::SharedAsset<ImageCesium>& image) {
  this->_pImageCesium = image;
}

TUniquePtr<FCesiumTextureResourceBase> createTextureResourceFromImageCesium(
    CesiumGltf::ImageCesium& imageCesium,
    TextureAddress addressX,
    TextureAddress addressY,
    TextureFilter filter,
    bool useMipMapsIfAvailable,
    TextureGroup group,
    bool sRGB,
    EPixelFormat pixelFormat) {
  if (GRHISupportsAsyncTextureCreation && !imageCesium.pixelData.empty()) {
    // Create RHI texture resource on this worker
    // thread, and then hand it off to the renderer
    // thread.
    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::CreateRHITexture2D)

    FTexture2DRHIRef textureReference =
        CreateRHITexture2D_Async(imageCesium, pixelFormat, sRGB);
    TUniquePtr<FCesiumTextureResourceBase> textureResource =
        MakeUnique<FCesiumUseExistingTextureResource>(
            textureReference,
            group,
            imageCesium.width,
            imageCesium.height,
            pixelFormat,
            filter,
            addressX,
            addressY,
            sRGB,
            useMipMapsIfAvailable,
            0);

    // Clear the now-unnecessary copy of the pixel data.
    // Calling clear() isn't good enough because it
    // won't actually release the memory.
    std::vector<std::byte> pixelData;
    imageCesium.pixelData.swap(pixelData);

    std::vector<CesiumGltf::ImageCesiumMipPosition> mipPositions;
    imageCesium.mipPositions.swap(mipPositions);

    return textureResource;
  } else {
    // The RHI texture will be created later on the
    // render thread, directly from this texture source.
    // We need valid pixelData here, though.
    if (imageCesium.pixelData.empty()) {
      return nullptr;
    }

    return MakeUnique<FCesiumCreateNewTextureResource>(
        imageCesium,
        group,
        imageCesium.width,
        imageCesium.height,
        pixelFormat,
        filter,
        addressX,
        addressY,
        sRGB,
        useMipMapsIfAvailable,
        0);
  }
}

static std::mutex textureResourceMutex;

struct ExtensionUnrealTextureResource {
  static inline constexpr const char* TypeName =
      "ExtensionUnrealTextureResource";
  static inline constexpr const char* ExtensionName =
      "PRIVATE_unreal_texture_resource";

  ExtensionUnrealTextureResource() {}

  TSharedPtr<FCesiumTextureResourceBase> pTextureResource = nullptr;

  // If a preprocessing step is required (such as generating mipmaps), this
  // future returns the preprocessed image. If no preprocessing is required,
  // this just passes the image through.
  std::optional<CesiumAsync::SharedFuture<CesiumGltf::ImageCesium*>>
      preprocessFuture = std::nullopt;

  std::optional<CesiumAsync::SharedFuture<FCesiumTextureResourceBase*>>
      resourceLoadingFuture = std::nullopt;

  static TUniquePtr<FCesiumTextureResourceBase> loadTextureResource(
      CesiumGltf::ImageCesium& imageCesium,
      TextureAddress addressX,
      TextureAddress addressY,
      TextureFilter filter,
      bool useMipMapsIfAvailable,
      TextureGroup group,
      bool sRGB,
      std::optional<EPixelFormat> overridePixelFormat) {
    std::optional<EPixelFormat> optionalPixelFormat =
        getPixelFormatForImageCesium(imageCesium, overridePixelFormat);
    if (!optionalPixelFormat.has_value()) {
      return nullptr;
    }

    EPixelFormat pixelFormat = optionalPixelFormat.value();

    std::lock_guard lock(textureResourceMutex);

    ExtensionUnrealTextureResource& extension =
        imageCesium.addExtension<ExtensionUnrealTextureResource>();

    // Already have a texture resource, just use that.
    if (extension.pTextureResource != nullptr) {
      return MakeUnique<FCesiumUseExistingTextureResource>(
          extension.pTextureResource.Get(),
          group,
          imageCesium.width,
          imageCesium.height,
          pixelFormat,
          filter,
          addressX,
          addressY,
          sRGB,
          useMipMapsIfAvailable,
          0);
    }

    // Store the current size of the pixel data, because
    // we're about to clear it but we still want to have
    // an accurate estimation of the size of the image for
    // caching purposes.
    imageCesium.sizeBytes = int64_t(imageCesium.pixelData.size());

    TUniquePtr<FCesiumTextureResourceBase> textureResource =
        createTextureResourceFromImageCesium(
            imageCesium,
            addressX,
            addressY,
            filter,
            useMipMapsIfAvailable,
            group,
            sRGB,
            pixelFormat);

    check(textureResource != nullptr);

    extension.pTextureResource =
        TSharedPtr<FCesiumTextureResourceBase>(textureResource.Release());
    return MakeUnique<FCesiumUseExistingTextureResource>(
        extension.pTextureResource.Get(),
        group,
        imageCesium.width,
        imageCesium.height,
        pixelFormat,
        filter,
        addressX,
        addressY,
        sRGB,
        useMipMapsIfAvailable,
        0);
  }
};

std::optional<int32_t> getSourceIndexFromModelAndTexture(
    const CesiumGltf::Model& model,
    const CesiumGltf::Texture& texture) {
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
      return std::nullopt;
    }
    return std::optional(pKtxExtension->source);
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
      return std::nullopt;
    }
    return std::optional(pWebpExtension->source);
  } else {
    if (texture.source < 0 || texture.source >= model.images.size()) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT(
              "Texture source index must be non-negative and less than %d, but is %d"),
          model.images.size(),
          texture.source);
      return std::nullopt;
    }
    return std::optional(texture.source);
  }
}

TUniquePtr<LoadedTextureResult> loadTextureFromModelAnyThreadPart(
    CesiumGltf::Model& model,
    CesiumGltf::Texture& texture,
    bool sRGB) {
  int64_t textureIndex =
      model.textures.empty() ? -1 : &texture - &model.textures[0];
  if (textureIndex < 0 || size_t(textureIndex) >= model.textures.size()) {
    textureIndex = -1;
  }

  std::lock_guard lock(textureResourceMutex);

  ExtensionUnrealTexture& extension =
      texture.addExtension<ExtensionUnrealTexture>();
  if (extension.pTexture && (extension.pTexture->getUnrealTexture() ||
                             extension.pTexture->getTextureResource())) {
    // There's an existing Unreal texture for this glTF texture. This will
    // happen if this texture is used by multiple primitives on the same
    // model. It will also be the case when this model was upsampled from a
    // parent tile.
    TUniquePtr<LoadedTextureResult> pResult = MakeUnique<LoadedTextureResult>();
    pResult->pTexture = extension.pTexture;
    pResult->textureIndex = textureIndex;
    return pResult;
  }

  std::optional<int32_t> optionalSourceIndex =
      getSourceIndexFromModelAndTexture(model, texture);
  if (!optionalSourceIndex.has_value()) {
    return nullptr;
  };

  CesiumGltf::Image& image = model.images[*optionalSourceIndex];
  const CesiumGltf::Sampler& sampler =
      model.getSafe(model.samplers, texture.sampler);

  TUniquePtr<LoadedTextureResult> result =
      loadTextureFromImageAndSamplerAnyThreadPart(image.cesium, sampler, sRGB);

  if (result) {
    std::lock_guard lock(textureResourceMutex);
    extension.pTexture = result->pTexture;
    result->textureIndex = textureIndex;
  }

  return result;
}

TextureFilter getTextureFilterFromSampler(const CesiumGltf::Sampler& sampler) {
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
    return sampler.magFilter.value() == CesiumGltf::Sampler::MagFilter::NEAREST
               ? TextureFilter::TF_Nearest
               : TextureFilter::TF_Default;
  } else if (sampler.minFilter) {
    // Use specified minFilter.
    switch (sampler.minFilter.value()) {
    case CesiumGltf::Sampler::MinFilter::NEAREST:
    case CesiumGltf::Sampler::MinFilter::NEAREST_MIPMAP_NEAREST:
      return TextureFilter::TF_Nearest;
    case CesiumGltf::Sampler::MinFilter::LINEAR:
    case CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_NEAREST:
      return TextureFilter::TF_Bilinear;
    default:
      return TextureFilter::TF_Default;
    }
  } else {
    // No filtering specified at all, let the texture group decide.
    return TextureFilter::TF_Default;
  }
}

bool getUseMipmapsIfAvailableFromSampler(const CesiumGltf::Sampler& sampler) {
  switch (sampler.minFilter.value_or(
      CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_LINEAR)) {
  case CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_LINEAR:
  case CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_NEAREST:
  case CesiumGltf::Sampler::MinFilter::NEAREST_MIPMAP_LINEAR:
  case CesiumGltf::Sampler::MinFilter::NEAREST_MIPMAP_NEAREST:
    return true;
    break;
  default: // LINEAR and NEAREST
    return false;
  }
}

TUniquePtr<LoadedTextureResult> loadTextureFromImageAndSamplerAnyThreadPart(
    CesiumGltf::SharedAsset<CesiumGltf::ImageCesium>& image,
    const CesiumGltf::Sampler& sampler,
    bool sRGB) {
  return loadTextureAnyThreadPart(
      image,
      convertGltfWrapSToUnreal(sampler.wrapS),
      convertGltfWrapTToUnreal(sampler.wrapT),
      getTextureFilterFromSampler(sampler),
      getUseMipmapsIfAvailableFromSampler(sampler),
      // TODO: allow texture group to be configured on Cesium3DTileset.
      TEXTUREGROUP_World,
      sRGB,
      std::nullopt);
}

TUniquePtr<LoadedTextureResult> loadTextureFromImageAndSamplerAnyThreadPartSync(
    CesiumGltf::ImageCesium& image,
    const CesiumGltf::Sampler& sampler,
    bool sRGB) {
  return loadTextureAnyThreadPartSync(
      image,
      convertGltfWrapSToUnreal(sampler.wrapS),
      convertGltfWrapTToUnreal(sampler.wrapT),
      getTextureFilterFromSampler(sampler),
      getUseMipmapsIfAvailableFromSampler(sampler),
      // TODO: allow texture group to be configured on Cesium3DTileset.
      TEXTUREGROUP_World,
      sRGB,
      std::nullopt);
}

static UTexture2D* CreateTexture2D(LoadedTextureResult* pHalfLoadedTexture) {
  if (!pHalfLoadedTexture || !pHalfLoadedTexture->pTexture) {
    return nullptr;
  }

  UTexture2D* pTexture = pHalfLoadedTexture->pTexture->getUnrealTexture();
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

    pHalfLoadedTexture->pTexture->setUnrealTexture(pTexture);
  }

  return pTexture;
}

TUniquePtr<LoadedTextureResult> loadTextureAnyThreadPart(
    CesiumGltf::SharedAsset<CesiumGltf::ImageCesium>& image,
    TextureAddress addressX,
    TextureAddress addressY,
    TextureFilter filter,
    bool useMipMapsIfAvailable,
    TextureGroup group,
    bool sRGB,
    std::optional<EPixelFormat> overridePixelFormat) {
  TUniquePtr<FCesiumTextureResourceBase> textureResource =
      ExtensionUnrealTextureResource::loadTextureResource(
          *image,
          addressX,
          addressY,
          filter,
          useMipMapsIfAvailable,
          group,
          sRGB,
          overridePixelFormat);

  TUniquePtr<LoadedTextureResult> pResult = MakeUnique<LoadedTextureResult>();
  pResult->pTexture = new ReferenceCountedUnrealTexture();

  pResult->addressX = addressX;
  pResult->addressY = addressY;
  pResult->filter = filter;
  pResult->group = group;
  pResult->sRGB = sRGB;

  pResult->pTexture->setTextureResource(MoveTemp(textureResource));
  pResult->pTexture->setSharedImage(image);
  return pResult;
}

TUniquePtr<LoadedTextureResult> loadTextureAnyThreadPartSync(
    CesiumGltf::ImageCesium& image,
    TextureAddress addressX,
    TextureAddress addressY,
    TextureFilter filter,
    bool useMipMapsIfAvailable,
    TextureGroup group,
    bool sRGB,
    std::optional<EPixelFormat> overridePixelFormat) {
  std::optional<EPixelFormat> optionalPixelFormat =
      getPixelFormatForImageCesium(image, overridePixelFormat);
  if (!optionalPixelFormat.has_value()) {
    return nullptr;
  }

  EPixelFormat pixelFormat = optionalPixelFormat.value();
  TUniquePtr<FCesiumTextureResourceBase> textureResource =
      createTextureResourceFromImageCesium(
          image,
          addressX,
          addressY,
          filter,
          useMipMapsIfAvailable,
          group,
          sRGB,
          pixelFormat);

  TUniquePtr<LoadedTextureResult> pResult = MakeUnique<LoadedTextureResult>();
  pResult->pTexture = new ReferenceCountedUnrealTexture();

  pResult->addressX = addressX;
  pResult->addressY = addressY;
  pResult->filter = filter;
  pResult->group = group;
  pResult->sRGB = sRGB;

  pResult->pTexture->setTextureResource(MoveTemp(textureResource));
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

  TUniquePtr<FCesiumTextureResourceBase>& pTextureResource =
      pHalfLoadedTexture->pTexture->getTextureResource();
  if (pTextureResource == nullptr) {
    // Texture is already loaded (or unloadable).
    return pHalfLoadedTexture->pTexture;
  }

  UTexture2D* pTexture = CreateTexture2D(pHalfLoadedTexture);
  if (pTexture == nullptr) {
    return nullptr;
  }

  FCesiumTextureResourceBase* pCesiumTextureResource =
      pTextureResource.Release();
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

TextureAddress convertGltfWrapSToUnreal(int32_t wrapS) {
  // glTF spec: "When undefined, a sampler with repeat wrapping and auto
  // filtering should be used."
  switch (wrapS) {
  case CesiumGltf::Sampler::WrapS::CLAMP_TO_EDGE:
    return TextureAddress::TA_Clamp;
  case CesiumGltf::Sampler::WrapS::MIRRORED_REPEAT:
    return TextureAddress::TA_Mirror;
  case CesiumGltf::Sampler::WrapS::REPEAT:
  default:
    return TextureAddress::TA_Wrap;
  }
}

TextureAddress convertGltfWrapTToUnreal(int32_t wrapT) {
  // glTF spec: "When undefined, a sampler with repeat wrapping and auto
  // filtering should be used."
  switch (wrapT) {
  case CesiumGltf::Sampler::WrapT::CLAMP_TO_EDGE:
    return TextureAddress::TA_Clamp;
  case CesiumGltf::Sampler::WrapT::MIRRORED_REPEAT:
    return TextureAddress::TA_Mirror;
  case CesiumGltf::Sampler::WrapT::REPEAT:
  default:
    return TextureAddress::TA_Wrap;
  }
}

CesiumAsync::SharedFuture<CesiumGltf::ImageCesium*> createMipMapsForSampler(
    const CesiumAsync::AsyncSystem& asyncSystem,
    const CesiumGltf::Sampler& sampler,
    CesiumGltf::ImageCesium& image) {
  std::unique_lock lock(textureResourceMutex);

  ExtensionUnrealTextureResource& extension =
      image.addExtension<ExtensionUnrealTextureResource>();

  // Future already exists, we don't need to do anything else.
  if (extension.preprocessFuture.has_value()) {
    return extension.preprocessFuture.value();
  }

  CesiumAsync::Promise<CesiumGltf::ImageCesium*> promise =
      asyncSystem.createPromise<CesiumGltf::ImageCesium*>();

  extension.preprocessFuture = promise.getFuture().share();

  lock.unlock();

  // Generate mipmaps if needed.
  // An image needs mipmaps generated for it if:
  // 1. It is used by a Texture that has a Sampler with a mipmap filtering
  // mode, and
  // 2. It does not already have mipmaps.
  // It's ok if an image has mipmaps even if not all textures will use them.
  // There's no reason to have two RHI textures, one with and one without
  // mips.
  bool needsMipmaps;
  switch (sampler.minFilter.value_or(
      CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_LINEAR)) {
  case CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_LINEAR:
  case CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_NEAREST:
  case CesiumGltf::Sampler::MinFilter::NEAREST_MIPMAP_LINEAR:
  case CesiumGltf::Sampler::MinFilter::NEAREST_MIPMAP_NEAREST:
    needsMipmaps = true;
    break;
  default: // LINEAR and NEAREST
    needsMipmaps = false;
    break;
  }

  if (!needsMipmaps || image.pixelData.empty()) {
    promise.resolve(&image);
    return *extension.preprocessFuture;
  }

  // We need mipmaps generated.
  std::optional<std::string> errorMessage =
      CesiumGltfReader::GltfReader::generateMipMaps(image);
  if (errorMessage) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("%s"),
        UTF8_TO_TCHAR(errorMessage->c_str()));
  }
  promise.resolve(&image);
  return *extension.preprocessFuture;
}

CesiumAsync::SharedFuture<void> createMipMapsForAllTextures(
    const CesiumAsync::AsyncSystem& asyncSystem,
    CesiumGltf::Model& model) {
  std::vector<CesiumAsync::SharedFuture<CesiumGltf::ImageCesium*>> futures;
  for (const Texture& texture : model.textures) {
    futures.push_back(createMipMapsForSampler(
        asyncSystem,
        model.getSafe(model.samplers, texture.sampler),
        *model.images[texture.source].cesium));
  }

  return asyncSystem.all(std::move(futures))
      .thenImmediately(
          []([[maybe_unused]] std::vector<CesiumGltf::ImageCesium*>& results)
              -> void {})
      .share();
}

} // namespace CesiumTextureUtility
