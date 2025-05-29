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
#include "ExtensionImageAssetUnreal.h"
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
#include <CesiumGltf/ImageAsset.h>
#include <CesiumGltf/Ktx2TranscodeTargets.h>
#include <CesiumGltfReader/GltfReader.h>
#include <CesiumUtility/IntrusivePointer.h>

namespace {

struct ExtensionUnrealTexture {
  static inline constexpr const char* TypeName = "ExtensionUnrealTexture";
  static inline constexpr const char* ExtensionName = "PRIVATE_unreal_texture";

  CesiumUtility::IntrusivePointer<
      CesiumTextureUtility::ReferenceCountedUnrealTexture>
      pTexture = nullptr;
};

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

const FCesiumTextureResourceUniquePtr&
ReferenceCountedUnrealTexture::getTextureResource() const {
  return this->_pTextureResource;
}

FCesiumTextureResourceUniquePtr&
ReferenceCountedUnrealTexture::getTextureResource() {
  return this->_pTextureResource;
}

void ReferenceCountedUnrealTexture::setTextureResource(
    FCesiumTextureResourceUniquePtr&& p) {
  this->_pTextureResource = std::move(p);
}

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
  if (image.pAsset == nullptr) {
    return nullptr;
  }

  const CesiumGltf::Sampler& sampler =
      model.getSafe(model.samplers, texture.sampler);

  TUniquePtr<LoadedTextureResult> result =
      loadTextureFromImageAndSamplerAnyThreadPart(*image.pAsset, sampler, sRGB);

  if (result) {
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
  default: // LINEAR and NEAREST
    return false;
  }
}

TUniquePtr<LoadedTextureResult> loadTextureFromImageAndSamplerAnyThreadPart(
    CesiumGltf::ImageAsset& image,
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
    CesiumGltf::ImageAsset& image,
    TextureAddress addressX,
    TextureAddress addressY,
    TextureFilter filter,
    bool useMipMapsIfAvailable,
    TextureGroup group,
    bool sRGB,
    std::optional<EPixelFormat> overridePixelFormat) {
  // The FCesiumTextureResource for the ImageAsset should already be created at
  // this point, if it can be.
  const ExtensionImageAssetUnreal& extension =
      ExtensionImageAssetUnreal::getOrCreate(
          CesiumAsync::AsyncSystem(nullptr),
          image,
          sRGB,
          useMipMapsIfAvailable,
          overridePixelFormat);
  check(extension.getFuture().isReady());
  if (extension.getTextureResource() == nullptr) {
    return nullptr;
  }

  auto pResource = FCesiumTextureResource::CreateWrapped(
      extension.getTextureResource(),
      group,
      filter,
      addressX,
      addressY,
      sRGB,
      useMipMapsIfAvailable);

  TUniquePtr<LoadedTextureResult> pResult = MakeUnique<LoadedTextureResult>();
  pResult->pTexture = new ReferenceCountedUnrealTexture();

  pResult->addressX = addressX;
  pResult->addressY = addressY;
  pResult->filter = filter;
  pResult->group = group;
  pResult->sRGB = sRGB;
  pResult->pTexture->setTextureResource(MoveTemp(pResource));

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

  FCesiumTextureResourceUniquePtr& pTextureResource =
      pHalfLoadedTexture->pTexture->getTextureResource();
  if (pTextureResource == nullptr) {
    // Texture is already loaded (or unloadable).
    return pHalfLoadedTexture->pTexture;
  }

  UTexture2D* pTexture = CreateTexture2D(pHalfLoadedTexture);
  if (pTexture == nullptr) {
    return nullptr;
  }

  if (pTextureResource) {
    // Give the UTexture2D exclusive ownership of this FCesiumTextureResource.
    pTexture->SetResource(pTextureResource.Release());

    ENQUEUE_RENDER_COMMAND(Cesium_InitResource)
    ([pTexture, pTextureResource = pTexture->GetResource()](
         FRHICommandListImmediate& RHICmdList) {
      pTextureResource->SetTextureReference(
          pTexture->TextureReference.TextureReferenceRHI);
      pTextureResource->InitResource(
          FRHICommandListImmediate::Get()); // Init Resource now requires a
                                            // command list.
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

std::optional<EPixelFormat> getPixelFormatForImageAsset(
    const CesiumGltf::ImageAsset& imageCesium,
    const std::optional<EPixelFormat> overridePixelFormat) {
  if (imageCesium.compressedPixelFormat !=
      CesiumGltf::GpuCompressedPixelFormat::NONE) {
    switch (imageCesium.compressedPixelFormat) {
    case CesiumGltf::GpuCompressedPixelFormat::ETC1_RGB:
      return EPixelFormat::PF_ETC1;
      break;
    case CesiumGltf::GpuCompressedPixelFormat::ETC2_RGBA:
      return EPixelFormat::PF_ETC2_RGBA;
      break;
    case CesiumGltf::GpuCompressedPixelFormat::BC1_RGB:
      return EPixelFormat::PF_DXT1;
      break;
    case CesiumGltf::GpuCompressedPixelFormat::BC3_RGBA:
      return EPixelFormat::PF_DXT5;
      break;
    case CesiumGltf::GpuCompressedPixelFormat::BC4_R:
      return EPixelFormat::PF_BC4;
      break;
    case CesiumGltf::GpuCompressedPixelFormat::BC5_RG:
      return EPixelFormat::PF_BC5;
      break;
    case CesiumGltf::GpuCompressedPixelFormat::BC7_RGBA:
      return EPixelFormat::PF_BC7;
      break;
    case CesiumGltf::GpuCompressedPixelFormat::ASTC_4x4_RGBA:
      return EPixelFormat::PF_ASTC_4x4;
      break;
    case CesiumGltf::GpuCompressedPixelFormat::PVRTC2_4_RGBA:
      return EPixelFormat::PF_PVRTC2;
      break;
    case CesiumGltf::GpuCompressedPixelFormat::ETC2_EAC_R11:
      return EPixelFormat::PF_ETC2_R11_EAC;
      break;
    case CesiumGltf::GpuCompressedPixelFormat::ETC2_EAC_RG11:
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

} // namespace CesiumTextureUtility
