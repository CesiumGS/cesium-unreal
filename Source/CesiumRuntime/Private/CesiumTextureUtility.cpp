// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumTextureUtility.h"
#include "CesiumRuntime.h"
#include "PixelFormat.h"
#include <CesiumGltf/ExtensionKhrTextureBasisu.h>
#include <CesiumGltf/ImageCesium.h>
#include <CesiumGltf/Ktx2TranscodeTargets.h>
#include <CesiumUtility/Tracing.h>
#include <stb_image_resize.h>

using namespace CesiumGltf;

namespace CesiumTextureUtility {

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

TUniquePtr<LoadedTextureResult> loadTextureAnyThreadPart(
    const CesiumGltf::ImageCesium& image,
    const TextureAddress& addressX,
    const TextureAddress& addressY,
    const TextureFilter& filter,
    const TextureGroup& group,
    bool generateMipMaps,
    bool sRGB) {

  CESIUM_TRACE("loadTextureAnyThreadPart");

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

  if (!image.mipPositions.empty()) {
    int32_t width = image.width;
    int32_t height = image.height;

    CESIUM_TRACE("Copying existing mips.");

    for (const CesiumGltf::ImageCesiumMipPosition& mip : image.mipPositions) {
      if (mip.byteOffset >= image.pixelData.size() ||
          mip.byteOffset + mip.byteSize > image.pixelData.size()) {
        UE_LOG(
            LogCesium,
            Warning,
            TEXT(
                "Invalid mip in glTF; it has a byteOffset of %d and a byteSize of %d but only %d bytes of pixel data are available."),
            mip.byteOffset,
            mip.byteSize,
            image.pixelData.size());
        continue;
      }

      FTexture2DMipMap* pLevel = new FTexture2DMipMap();
      pResult->pTextureData->Mips.Add(pLevel);

      pLevel->SizeX = width;
      pLevel->SizeY = height;
      pLevel->BulkData.Lock(LOCK_READ_WRITE);

      void* pMipData = pLevel->BulkData.Realloc(mip.byteSize);
      FMemory::Memcpy(
          pMipData,
          image.pixelData.data() + mip.byteOffset,
          mip.byteSize);

      width >>= 1;
      if (width == 0) {
        width = 1;
      }

      height >>= 1;
      if (height == 0) {
        height = 1;
      }
    }
  } else {
    int32_t width = image.width;
    int32_t height = image.height;
    int32_t channels = image.channels;

    void* pLastMipData = nullptr;
    {
      CESIUM_TRACE("Copying image.");

      // Create level 0 mip (full res image)
      FTexture2DMipMap* pLevel0 = new FTexture2DMipMap();
      pResult->pTextureData->Mips.Add(pLevel0);
      pLevel0->SizeX = width;
      pLevel0->SizeY = height;
      pLevel0->BulkData.Lock(LOCK_READ_WRITE);

      pLastMipData = pLevel0->BulkData.Realloc(image.pixelData.size());
      FMemory::Memcpy(
          pLastMipData,
          image.pixelData.data(),
          image.pixelData.size());
    }

    if (generateMipMaps) {
      CESIUM_TRACE("Generate new mips.");

      // Generate mip levels.
      // TODO: do this on the GPU?
      while (width > 1 || height > 1) {
        FTexture2DMipMap* pLevel = new FTexture2DMipMap();
        pResult->pTextureData->Mips.Add(pLevel);

        pLevel->SizeX = width >> 1;
        if (pLevel->SizeX < 1)
          pLevel->SizeX = 1;
        pLevel->SizeY = height >> 1;
        if (pLevel->SizeY < 1)
          pLevel->SizeY = 1;

        pLevel->BulkData.Lock(LOCK_READ_WRITE);

        void* pMipData =
            pLevel->BulkData.Realloc(pLevel->SizeX * pLevel->SizeY * channels);

        // TODO: Premultiplied alpha? Cases with more than one byte per channel?
        // Non-normalzied pixel formats?
        if (!stbir_resize_uint8(
                static_cast<const unsigned char*>(pLastMipData),
                width,
                height,
                0,
                static_cast<unsigned char*>(pMipData),
                pLevel->SizeX,
                pLevel->SizeY,
                0,
                channels)) {
          // Failed to generate mip level, use bilinear filtering instead.
          pResult->filter = TextureFilter::TF_Bilinear;
          for (int32_t i = 1; i < pResult->pTextureData->Mips.Num(); ++i) {
            pResult->pTextureData->Mips[i].BulkData.Unlock();
          }
          pResult->pTextureData->Mips.RemoveAt(
              1,
              pResult->pTextureData->Mips.Num() - 1);
          break;
        }

        width = pLevel->SizeX;
        height = pLevel->SizeY;
        pLastMipData = pMipData;
      }
    }
  }

  // Unlock all levels
  for (int32_t i = 0; i < pResult->pTextureData->Mips.Num(); ++i) {
    pResult->pTextureData->Mips[i].BulkData.Unlock();
  }

  return pResult;
}

TUniquePtr<LoadedTextureResult> loadTextureAnyThreadPart(
    const CesiumGltf::Model& model,
    const CesiumGltf::Texture& texture,
    bool sRGB) {

  const CesiumGltf::ExtensionKhrTextureBasisu* pKtxExtension =
      texture.getExtension<CesiumGltf::ExtensionKhrTextureBasisu>();

  if (pKtxExtension) {
    if (pKtxExtension->source < 0 ||
        pKtxExtension->source >= model.images.size()) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT(
              "KTX texture source index must be non-negative and less than %d, but is %d"),
          model.images.size(),
          texture.source);
      return nullptr;
    }
  } else if (texture.source < 0 || texture.source >= model.images.size()) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT(
            "Texture source index must be non-negative and less than %d, but is %d"),
        model.images.size(),
        texture.source);
    return nullptr;
  }

  const CesiumGltf::ImageCesium& image =
      model.images[pKtxExtension ? pKtxExtension->source : texture.source]
          .cesium;
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

  return loadTextureAnyThreadPart(
      image,
      addressX,
      addressY,
      filter,
      TextureGroup::TEXTUREGROUP_World,
      useMipMaps,
      sRGB);
}

UTexture2D* loadTextureGameThreadPart(LoadedTextureResult* pHalfLoadedTexture) {
  if (!pHalfLoadedTexture) {
    return nullptr;
  }

  UTexture2D* pTexture = pHalfLoadedTexture->pTexture.Get();
  if (!pTexture && pHalfLoadedTexture->pTextureData) {
    pTexture = NewObject<UTexture2D>(
        GetTransientPackage(),
        NAME_None,
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
    pTexture->UpdateResource();

    pHalfLoadedTexture->pTexture = pTexture;
  }

  return pTexture;
}
} // namespace CesiumTextureUtility
