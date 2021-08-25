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

/*static*/ CesiumTextureUtility::LoadedTextureResult*
CesiumTextureUtility::loadTextureAnyThreadPart(
    const CesiumGltf::ImageCesium& image,
    const TextureAddress& addressX,
    const TextureAddress& addressY,
    const TextureFilter& filter) {

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

  LoadedTextureResult* pResult = new LoadedTextureResult{};
  pResult->pTextureData =
      createTexturePlatformData(image.width, image.height, pixelFormat);
  if (!pResult->pTextureData) {
    return nullptr;
  }
  pResult->addressX = addressX;
  pResult->addressY = addressY;
  pResult->filter = filter;

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

/*static*/ CesiumTextureUtility::LoadedTextureResult*
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

  // glTF spec: "When undefined, a sampler with repeat wrapping and auto
  // filtering should be used."
  TextureAddress addressX = TextureAddress::TA_Wrap;
  TextureAddress addressY = TextureAddress::TA_Wrap;
  ;
  TextureFilter filter = TextureFilter::TF_Default;

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

    // Unreal Engine's available filtering modes are only nearest, bilinear, and
    // trilinear, and are not specified separately for minification and
    // magnification. So we get as close as we can.
    if (!pSampler->minFilter && !pSampler->magFilter) {
      filter = TextureFilter::TF_Default;
    } else if (
        (!pSampler->minFilter ||
         pSampler->minFilter == CesiumGltf::Sampler::MinFilter::NEAREST) &&
        (!pSampler->magFilter ||
         pSampler->magFilter == CesiumGltf::Sampler::MagFilter::NEAREST)) {
      filter = TextureFilter::TF_Nearest;
    } else if (pSampler->minFilter) {
      switch (pSampler->minFilter.value()) {
      case CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_LINEAR:
      case CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_NEAREST:
      case CesiumGltf::Sampler::MinFilter::NEAREST_MIPMAP_LINEAR:
      case CesiumGltf::Sampler::MinFilter::NEAREST_MIPMAP_NEAREST:
        filter = TextureFilter::TF_Trilinear;
        break;
      default:
        filter = TextureFilter::TF_Bilinear;
        break;
      }
    } else if (pSampler->magFilter) {
      filter = pSampler->magFilter == CesiumGltf::Sampler::MagFilter::LINEAR
                   ? TextureFilter::TF_Bilinear
                   : TextureFilter::TF_Nearest;
    }
  }

  return loadTextureAnyThreadPart(image, addressX, addressY, filter);
}

/*static*/ bool CesiumTextureUtility::loadTextureGameThreadPart(
    LoadedTextureResult* pHalfLoadedTexture) {
  if (!pHalfLoadedTexture) {
    return false;
  }

  UTexture2D*& pTexture = pHalfLoadedTexture->pTexture;

  if (!pTexture) {
    pTexture = NewObject<UTexture2D>(
        GetTransientPackage(),
        NAME_None,
        RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);

    pTexture->PlatformData = pHalfLoadedTexture->pTextureData;
    pTexture->AddressX = pHalfLoadedTexture->addressX;
    pTexture->AddressY = pHalfLoadedTexture->addressY;
    pTexture->Filter = pHalfLoadedTexture->filter;
    pTexture->UpdateResource();
  }

  return true;
}
