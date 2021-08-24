// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumTextureUtility.h"
#include "PixelFormat.h"

#include <stb_image_resize.h>

static FTexturePlatformData*
createTexturePlatformData(int32 sizeX, int32 sizeY, EPixelFormat format) {
  if (sizeX > 0 && sizeY > 0 &&
      (sizeX % GPixelFormats[format].BlockSizeX) == 0 &&
      (sizeY % GPixelFormats[format].BlockSizeY) == 0) {
    FTexturePlatformData* pTexturePlatformData = new FTexturePlatformData();
    pTexturePlatformData->SizeX = sizeX;
    pTexturePlatformData->SizeY = sizeY;
    pTexturePlatformData->PixelFormat = format;

    // Allocate first mipmap.
    int32 NumBlocksX = sizeX / GPixelFormats[format].BlockSizeX;
    int32 NumBlocksY = sizeY / GPixelFormats[format].BlockSizeY;
    FTexture2DMipMap* Mip = new FTexture2DMipMap();
    pTexturePlatformData->Mips.Add(Mip);
    Mip->SizeX = sizeX;
    Mip->SizeY = sizeY;
    Mip->BulkData.Lock(LOCK_READ_WRITE);
    Mip->BulkData.Realloc(
        NumBlocksX * NumBlocksY * GPixelFormats[format].BlockBytes);
    Mip->BulkData.Unlock();

    return pTexturePlatformData;
  } else {
    return nullptr;
  }
}

/*static*/ CesiumTextureUtility::HalfLoadedTexture*
CesiumTextureUtility::loadTextureAnyThreadPart(
    const CesiumGltf::ImageCesium& image,
    const std::optional<CesiumGltf::Sampler>& sampler) {

  EPixelFormat pixelFormat;
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

  HalfLoadedTexture* pResult = new HalfLoadedTexture{};
  pResult->pTextureData =
      createTexturePlatformData(image.width, image.height, pixelFormat);
  if (!pResult->pTextureData) {
    return nullptr;
  }

  if (sampler) {
    switch (sampler->wrapS) {
    case CesiumGltf::Sampler::WrapS::CLAMP_TO_EDGE:
      pResult->addressX = TextureAddress::TA_Clamp;
      break;
    case CesiumGltf::Sampler::WrapS::MIRRORED_REPEAT:
      pResult->addressX = TextureAddress::TA_Mirror;
      break;
    case CesiumGltf::Sampler::WrapS::REPEAT:
      pResult->addressX = TextureAddress::TA_Wrap;
      break;
    }

    switch (sampler->wrapT) {
    case CesiumGltf::Sampler::WrapT::CLAMP_TO_EDGE:
      pResult->addressY = TextureAddress::TA_Clamp;
      break;
    case CesiumGltf::Sampler::WrapT::MIRRORED_REPEAT:
      pResult->addressY = TextureAddress::TA_Mirror;
      break;
    case CesiumGltf::Sampler::WrapT::REPEAT:
      pResult->addressY = TextureAddress::TA_Wrap;
      break;
    }

    // Unreal Engine's available filtering modes are only nearest, bilinear, and
    // trilinear, and are not specified separately for minification and
    // magnification. So we get as close as we can.
    if (!sampler->minFilter && !sampler->magFilter) {
      pResult->filter = TextureFilter::TF_Default;
    } else if (
        (!sampler->minFilter ||
         sampler->minFilter == CesiumGltf::Sampler::MinFilter::NEAREST) &&
        (!sampler->magFilter ||
         sampler->magFilter == CesiumGltf::Sampler::MagFilter::NEAREST)) {
      pResult->filter = TextureFilter::TF_Nearest;
    } else if (sampler->minFilter) {
      switch (sampler->minFilter.value()) {
      case CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_LINEAR:
      case CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_NEAREST:
      case CesiumGltf::Sampler::MinFilter::NEAREST_MIPMAP_LINEAR:
      case CesiumGltf::Sampler::MinFilter::NEAREST_MIPMAP_NEAREST:
        pResult->filter = TextureFilter::TF_Trilinear;
        break;
      default:
        pResult->filter = TextureFilter::TF_Bilinear;
        break;
      }
    } else if (sampler->magFilter) {
      pResult->filter =
          sampler->magFilter.value() == CesiumGltf::Sampler::MagFilter::LINEAR
              ? TextureFilter::TF_Bilinear
              : TextureFilter::TF_Nearest;
    }
  } else {
    // glTF spec: "When undefined, a sampler with repeat wrapping and auto
    // filtering should be used."
    pResult->addressX = TextureAddress::TA_Wrap;
    pResult->addressY = TextureAddress::TA_Wrap;
    pResult->filter = TextureFilter::TF_Default;
  }

  void* pTextureData = static_cast<unsigned char*>(
      pResult->pTextureData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
  FMemory::Memcpy(pTextureData, image.pixelData.data(), image.pixelData.size());

  if (pResult->filter == TextureFilter::TF_Trilinear) {
    // Generate mip levels.
    // TODO: do this on the GPU?
    int32_t width = image.width;
    int32_t height = image.height;
    int32_t channels = image.channels;

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
              static_cast<const unsigned char*>(pTextureData),
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
      pTextureData = pMipData;
    }
  }

  // Unlock all levels
  for (int32_t i = 0; i < pResult->pTextureData->Mips.Num(); ++i) {
    pResult->pTextureData->Mips[i].BulkData.Unlock();
  }

  return pResult;
}

/*static*/ CesiumTextureUtility::HalfLoadedTexture*
CesiumTextureUtility::loadTextureAnyThreadPart(
    const CesiumGltf::Model& model,
    const CesiumGltf::Texture& texture) {

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

  const CesiumGltf::ImageCesium& image = model.images[texture.source].cesium;
  const CesiumGltf::Sampler* pSampler =
      CesiumGltf::Model::getSafe(&model.samplers, texture.sampler);

  return loadTextureAnyThreadPart(
      image,
      pSampler ? std::make_optional(*pSampler) : std::nullopt);
}

/*static*/ UTexture2D* CesiumTextureUtility::loadTextureGameThreadPart(
    HalfLoadedTexture* pHalfLoadedTexture) {
  if (!pHalfLoadedTexture) {
    return nullptr;
  }

  UTexture2D* pTexture = NewObject<UTexture2D>(
      GetTransientPackage(),
      NAME_None,
      RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);

  pTexture->PlatformData = pHalfLoadedTexture->pTextureData;
  pTexture->AddressX = pHalfLoadedTexture->addressX;
  pTexture->AddressY = pHalfLoadedTexture->addressY;
  pTexture->Filter = pHalfLoadedTexture->filter;
  pTexture->UpdateResource();

  delete pHalfLoadedTexture;

  return pTexture;
}
