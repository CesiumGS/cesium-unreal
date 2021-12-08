// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumTextureUtility.h"
#include "CesiumFeatureIDTexture.h"
#include "CesiumMetadataArray.h"
#include "CesiumMetadataFeatureTable.h"
#include "CesiumMetadataPrimitive.h"
#include "CesiumMetadataProperty.h"
#include "CesiumRuntime.h"
#include "CesiumVertexMetadata.h"
#include "PixelFormat.h"

#include "CesiumGltf/FeatureIDTextureView.h"

#include "Containers/Map.h"

#include <cassert>
#include <stb_image_resize.h>
#include <unordered_map>

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

static constexpr size_t
getNonArrayPropertySize(ECesiumMetadataBlueprintType type) {
  switch (type) {
  case ECesiumMetadataBlueprintType::Boolean:
    // TODO: should booleans be 1 byte??
  case ECesiumMetadataBlueprintType::Byte:
    return 1;
  case ECesiumMetadataBlueprintType::Integer:
    return 4;
  case ECesiumMetadataBlueprintType::Integer64:
    return 8;
  case ECesiumMetadataBlueprintType::Float:
    return 4;
  default:
    return 0;
  }
}

namespace {

struct EncodedPixelFormat {
  EPixelFormat format;
  size_t pixelSize;
};

// TODO: revisit this once a type unpacking node is implemented in materials.
// We may be able to pack tighter than this in some cases at the cost of
// having confusing channel orders. If there is an interface in place to handle
// switching channels back, reinterpreting to correct type, etc., that won't be
// a problem.
// TODO: check platform specific support, ideally only encode metadata into
// universally accepted raw pixel formats
EncodedPixelFormat getPixelFormat(
    ECesiumMetadataBlueprintType type,
    ECesiumMetadataBlueprintType componentType,
    int64 componentCount) {
  switch (type) {
  case ECesiumMetadataBlueprintType::Boolean:
  case ECesiumMetadataBlueprintType::Byte:
    // needs to be reinterpreted as signed byte in shader
    return {EPixelFormat::PF_R8_UINT, 1};
  case ECesiumMetadataBlueprintType::Integer:
    return {EPixelFormat::PF_R32_SINT, 4};
  case ECesiumMetadataBlueprintType::Float:
    return {EPixelFormat::PF_R32_FLOAT, 4};
  case ECesiumMetadataBlueprintType::Array:
    switch (componentType) {
    case ECesiumMetadataBlueprintType::Boolean:
    case ECesiumMetadataBlueprintType::Byte:
      switch (componentCount) {
      case 2:
      case 3:
      case 4:
        // needs to be reinterpreted as signed ints in shader
        return {EPixelFormat::PF_R8G8B8A8_UINT, 4};
      default:
        return {EPixelFormat::PF_Unknown, 0};
      }
    case ECesiumMetadataBlueprintType::Integer:
      switch (componentCount) {
      case 2:
        // needs to be reinterpreted as signed ints in shader
        return {EPixelFormat::PF_R32G32_UINT, 8};
      case 3:
      case 4:
        // needs to be reinterpreted as signed ints in shader
        return {EPixelFormat::PF_R32G32B32A32_UINT, 16};
      default:
        return {EPixelFormat::PF_Unknown, 0};
      }
    case ECesiumMetadataBlueprintType::Float:
      switch (componentCount) {
      case 2:
        return {EPixelFormat::PF_G32R32F, 8};
      case 3:
      case 4:
        return {EPixelFormat::PF_A32B32G32R32F, 16};
      default:
        return {EPixelFormat::PF_Unknown, 0};
      }
    default:
      return {EPixelFormat::PF_Unknown, 0};
    }
  default:
    return {EPixelFormat::PF_Unknown, 0};
  }
}
} // namespace

/*static*/
CesiumTextureUtility::EncodedMetadataFeatureTable
CesiumTextureUtility::encodeMetadataFeatureTableAnyThreadPart(
    const FCesiumMetadataFeatureTable& featureTable) {

  EncodedMetadataFeatureTable encodedFeatureTable;

  int64 featureCount =
      UCesiumMetadataFeatureTableBlueprintLibrary::GetNumberOfFeatures(
          featureTable);

  const TMap<FString, FCesiumMetadataProperty>& properties =
      UCesiumMetadataFeatureTableBlueprintLibrary::GetProperties(featureTable);

  encodedFeatureTable.encodedProperties.reserve(properties.Num());
  for (const auto& pair : properties) {
    const FCesiumMetadataProperty& property = pair.Value;

    ECesiumMetadataBlueprintType type =
        UCesiumMetadataPropertyBlueprintLibrary::GetBlueprintType(property);
    ECesiumMetadataBlueprintType componentType =
        ECesiumMetadataBlueprintType::None;

    int64 propertySize = 0;
    int64 componentCount = 0;
    if (type == ECesiumMetadataBlueprintType::None ||
        type == ECesiumMetadataBlueprintType::Integer64 ||
        type == ECesiumMetadataBlueprintType::String) {
      continue;
    } else if (type == ECesiumMetadataBlueprintType::Array) {
      componentType =
          UCesiumMetadataPropertyBlueprintLibrary::GetBlueprintComponentType(
              property);

      if (componentType == ECesiumMetadataBlueprintType::None ||
          componentType == ECesiumMetadataBlueprintType::Integer64 ||
          componentType == ECesiumMetadataBlueprintType::String ||
          componentType == ECesiumMetadataBlueprintType::Array) {
        continue;
      }

      componentCount =
          UCesiumMetadataPropertyBlueprintLibrary::GetComponentCount(property);
      propertySize = getNonArrayPropertySize(componentType) * componentCount;

    } else {
      propertySize = getNonArrayPropertySize(type);
    }

    EncodedPixelFormat encodedFormat =
        getPixelFormat(type, componentType, componentCount);

    if (encodedFormat.format == EPixelFormat::PF_Unknown) {
      continue;
    }

    EncodedMetadataProperty& encodedProperty =
        encodedFeatureTable.encodedProperties.emplace_back();
    encodedProperty.name = pair.Key;
    encodedProperty.type = type;
    encodedProperty.componentType = componentType;

    int64 propertyArraySize = featureCount * propertySize;
    encodedProperty.pTexture = new LoadedTextureResult();
    encodedProperty.pTexture->pTextureData =
        createTexturePlatformData(propertyArraySize, 1, encodedFormat.format);

    encodedProperty.pTexture->addressX = TextureAddress::TA_Clamp;
    encodedProperty.pTexture->addressY = TextureAddress::TA_Clamp;
    // TODO: is this correct?
    encodedProperty.pTexture->filter = TextureFilter::TF_Nearest;

    if (!encodedProperty.pTexture->pTextureData) {
      // TODO: print error?
      break;
    }

    void* pTextureData = static_cast<unsigned char*>(
        encodedProperty.pTexture->pTextureData->Mips[0].BulkData.Lock(
            LOCK_READ_WRITE));

    // TODO: move to helper function?
    // TODO: can we copy directly from the original metadata buffer in some
    // cases?
    switch (type) {
    case ECesiumMetadataBlueprintType::Boolean:
    case ECesiumMetadataBlueprintType::Byte: {
      uint8* pWritePos =
          reinterpret_cast<uint8*>(encodedProperty.pTexture->pTextureData);
      for (int64 i = 0; i < featureCount; ++i) {
        *pWritePos =
            UCesiumMetadataPropertyBlueprintLibrary::GetByte(property, i);
        ++pWritePos;
      }
    } break;
    case ECesiumMetadataBlueprintType::Integer: {
      int32* pWritePos =
          reinterpret_cast<int32*>(encodedProperty.pTexture->pTextureData);
      for (int64 i = 0; i < featureCount; ++i) {
        *pWritePos =
            UCesiumMetadataPropertyBlueprintLibrary::GetInteger(property, i);
        ++pWritePos;
      }
    } break;
    case ECesiumMetadataBlueprintType::Float: {
      float* pWritePos =
          reinterpret_cast<float*>(encodedProperty.pTexture->pTextureData);
      for (int64 i = 0; i < featureCount; ++i) {
        *pWritePos =
            UCesiumMetadataPropertyBlueprintLibrary::GetFloat(property, i);
        ++pWritePos;
      }
    } break;
    case ECesiumMetadataBlueprintType::Array:
      switch (componentType) {
      case ECesiumMetadataBlueprintType::Boolean:
      case ECesiumMetadataBlueprintType::Byte: {
        uint8* pWritePos =
            reinterpret_cast<uint8*>(encodedProperty.pTexture->pTextureData);
        for (int64 i = 0; i < featureCount; ++i) {
          FCesiumMetadataArray arrayProperty =
              UCesiumMetadataPropertyBlueprintLibrary::GetArray(property, i);
          for (int64 j = 0; j < componentCount; ++j) {
            *(pWritePos + j) =
                UCesiumMetadataArrayBlueprintLibrary::GetByte(arrayProperty, j);
          }
          pWritePos += encodedFormat.pixelSize;
        }
      } break;
      case ECesiumMetadataBlueprintType::Integer: {
        uint8* pWritePos =
            reinterpret_cast<uint8*>(encodedProperty.pTexture->pTextureData);
        for (int64 i = 0; i < featureCount; ++i) {
          FCesiumMetadataArray arrayProperty =
              UCesiumMetadataPropertyBlueprintLibrary::GetArray(property, i);
          int32* pWritePosI = reinterpret_cast<int32*>(pWritePos);
          for (int64 j = 0; j < componentCount; ++j) {
            *pWritePosI = UCesiumMetadataArrayBlueprintLibrary::GetInteger(
                arrayProperty,
                j);
            ++pWritePosI;
          }
          pWritePos += encodedFormat.pixelSize;
        }
      } break;
      case ECesiumMetadataBlueprintType::Float: {
        uint8* pWritePos =
            reinterpret_cast<uint8*>(encodedProperty.pTexture->pTextureData);
        for (int64 i = 0; i < featureCount; ++i) {
          FCesiumMetadataArray arrayProperty =
              UCesiumMetadataPropertyBlueprintLibrary::GetArray(property, i);
          // Floats are encoded backwards (e.g., ABGR, GR)
          float* pWritePosF =
              reinterpret_cast<float*>(pWritePos + encodedFormat.pixelSize) - 1;
          for (int64 j = 0; j < componentCount; ++j) {
            *pWritePosF = UCesiumMetadataArrayBlueprintLibrary::GetFloat(
                arrayProperty,
                j);
            --pWritePosF;
          }
          pWritePos += encodedFormat.pixelSize;
        }
      } break;
      }
      break;
    }

    encodedProperty.pTexture->pTextureData->Mips[0].BulkData.Unlock();
  }

  return encodedFeatureTable;
}

/*static*/
CesiumTextureUtility::EncodedMetadataPrimitive
CesiumTextureUtility::encodeMetadataPrimitiveAnyThreadPart(
    const FCesiumMetadataPrimitive& primitive) {
  EncodedMetadataPrimitive result;

  const TArray<FCesiumFeatureIDTexture>& featureIdTextures =
      UCesiumMetadataPrimitiveBlueprintLibrary::GetFeatureIDTextures(primitive);
  const TArray<FCesiumVertexMetadata>& vertexFeatures =
      UCesiumMetadataPrimitiveBlueprintLibrary::GetVertexFeatures(primitive);

  // Multiple feature id textures can correspond to a single Cesium image. Keep
  // track of which Cesium image corresponds to which Unreal image, so we don't
  // duplicate.
  // TODO: be very careful about how the game thread part and deletion happens
  // for images used by multiple feature id textures. Maybe a shared_ptr or
  // something is needed.
  std::unordered_map<const CesiumGltf::ImageCesium*, LoadedTextureResult*>
      featureIdTextureMap;
  featureIdTextureMap.reserve(featureIdTextures.Num());

  result.encodedFeatureIdTextures.resize(featureIdTextures.Num());
  result.encodedVertexMetadata.resize(vertexFeatures.Num());

  // TODO: this assumes each feature table will only be used once in a
  // feature id texture (or later feature id attribute).
  // Actually feature tables should be encoded per-glTF and primitives
  // should be able to request it if it is not already loaded.
  for (size_t i = 0; i < featureIdTextures.Num(); ++i) {
    const FCesiumFeatureIDTexture& featureIdTexture = featureIdTextures[i];
    EncodedFeatureIdTexture& encodedFeatureIdTexture =
        result.encodedFeatureIdTextures[i];

    const FCesiumMetadataFeatureTable& featureTable =
        UCesiumFeatureIDTextureBlueprintLibrary::GetFeatureTable(
            featureIdTexture);
    const CesiumGltf::FeatureIDTextureView& featureIdTextureView =
        featureIdTexture.getFeatureIdTextureView();
    const CesiumGltf::ImageCesium* pFeatureIdImage =
        featureIdTextureView.getImage();

    if (!pFeatureIdImage) {
      continue;
    }

    encodedFeatureIdTexture.name =
        "FeatureIdTexture" + FString::FromInt(static_cast<int32>(i));
    encodedFeatureIdTexture.channel = featureIdTextureView.getChannel();
    encodedFeatureIdTexture.textureCoordinateIndex =
        featureIdTextureView.getTextureCoordinateIndex();

    auto mappedUnrealImage = featureIdTextureMap.find(pFeatureIdImage);
    if (mappedUnrealImage != featureIdTextureMap.end()) {
      encodedFeatureIdTexture.pTexture = mappedUnrealImage->second;
    } else {
      encodedFeatureIdTexture.pTexture = new LoadedTextureResult{};
      encodedFeatureIdTexture.pTexture->pTextureData =
          createTexturePlatformData(
              pFeatureIdImage->width,
              pFeatureIdImage->height,
              // TODO: currently this is always the case, but doesn't have to be
              // needs to be reinterpreted as signed ints in shader
              EPixelFormat::PF_R8G8B8A8 /*_UINT*/);

      encodedFeatureIdTexture.pTexture->addressX = TextureAddress::TA_Clamp;
      encodedFeatureIdTexture.pTexture->addressY = TextureAddress::TA_Clamp;
      encodedFeatureIdTexture.pTexture->filter = TextureFilter::TF_Nearest;

      if (!encodedFeatureIdTexture.pTexture->pTextureData) {
        continue;
      }

      void* pTextureData = static_cast<unsigned char*>(
          encodedFeatureIdTexture.pTexture->pTextureData->Mips[0].BulkData.Lock(
              LOCK_READ_WRITE));
      FMemory::Memcpy(
          pTextureData,
          pFeatureIdImage->pixelData.data(),
          pFeatureIdImage->pixelData.size());
      encodedFeatureIdTexture.pTexture->pTextureData->Mips[0].BulkData.Unlock();
    }

    encodedFeatureIdTexture.encodedFeatureTable =
        encodeMetadataFeatureTableAnyThreadPart(featureTable);
  }

  return result;
}

/*static*/
bool CesiumTextureUtility::encodeMetadataFeatureTableGameThreadPart(
    EncodedMetadataFeatureTable& encodedFeatureTable) {
  bool success = true;

  for (EncodedMetadataProperty& encodedProperty :
       encodedFeatureTable.encodedProperties) {
    success |= CesiumTextureUtility::loadTextureGameThreadPart(
        encodedProperty.pTexture);
  }

  return success;
}

/*static*/
bool CesiumTextureUtility::encodeMetadataPrimitiveGameThreadPart(
    EncodedMetadataPrimitive& encodedPrimitive) {
  bool success = true;

  std::vector<const LoadedTextureResult*> uniqueFeatureIdImages;
  uniqueFeatureIdImages.reserve(
      encodedPrimitive.encodedFeatureIdTextures.size());

  for (EncodedFeatureIdTexture& encodedFeatureIdTexture :
       encodedPrimitive.encodedFeatureIdTextures) {
    bool found = false;
    for (const LoadedTextureResult* pUniqueFeatureIdImage :
         uniqueFeatureIdImages) {
      if (pUniqueFeatureIdImage == encodedFeatureIdTexture.pTexture) {
        found = true;
        break;
      }
    }

    if (!found) {
      success |= CesiumTextureUtility::loadTextureGameThreadPart(
          encodedFeatureIdTexture.pTexture);
      uniqueFeatureIdImages.push_back(encodedFeatureIdTexture.pTexture);
    }

    success |= CesiumTextureUtility::encodeMetadataFeatureTableGameThreadPart(
        encodedFeatureIdTexture.encodedFeatureTable);
  }

  return success;
}
