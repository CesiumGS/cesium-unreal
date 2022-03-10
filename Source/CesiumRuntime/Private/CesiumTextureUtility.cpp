// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumTextureUtility.h"
#include "CesiumEncodedMetadataComponent.h"
#include "CesiumFeatureIDTexture.h"
#include "CesiumFeatureTexture.h"
#include "CesiumLifetime.h"
#include "CesiumMetadataArray.h"
#include "CesiumMetadataConversions.h"
#include "CesiumMetadataFeatureTable.h"
#include "CesiumMetadataModel.h"
#include "CesiumMetadataPrimitive.h"
#include "CesiumMetadataProperty.h"
#include "CesiumRuntime.h"
#include "CesiumVertexMetadata.h"
#include "Containers/Map.h"
#include "PixelFormat.h"
#include <CesiumGltf/ExtensionKhrTextureBasisu.h>
#include <CesiumGltf/FeatureIDTextureView.h>
#include <CesiumGltf/FeatureTexturePropertyView.h>
#include <CesiumGltf/FeatureTextureView.h>
#include <CesiumGltf/ImageCesium.h>
#include <CesiumGltf/Ktx2TranscodeTargets.h>
#include <CesiumUtility/Tracing.h>
#include <cassert>
#include <limits>
#include <stb_image_resize.h>
#include <unordered_map>

using namespace CesiumGltf;

namespace CesiumTextureUtility {

static FTexturePlatformData*
createTexturePlatformData(int32 sizeX, int32 sizeY, EPixelFormat format) {
  if (sizeX > 0 && sizeY > 0 &&
      (sizeX % GPixelFormats[format].BlockSizeX) == 0 &&
      (sizeY % GPixelFormats[format].BlockSizeY) == 0) {
    FTexturePlatformData* pTexturePlatformData = new FTexturePlatformData();
    pTexturePlatformData->SizeX = sizeX;
    pTexturePlatformData->SizeY = sizeY;
    pTexturePlatformData->PixelFormat = format;

    return pTexturePlatformData;
  } else {
    return nullptr;
  }
}

LoadedTextureResult* loadTextureAnyThreadPart(
    const CesiumGltf::ImageCesium& image,
    const TextureAddress& addressX,
    const TextureAddress& addressY,
    const TextureFilter& filter) {

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

  LoadedTextureResult* pResult = new LoadedTextureResult{};
  pResult->pTextureData =
      createTexturePlatformData(image.width, image.height, pixelFormat);
  if (!pResult->pTextureData) {
    return nullptr;
  }

  pResult->addressX = addressX;
  pResult->addressY = addressY;
  pResult->filter = filter;

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

    if (pResult->filter == TextureFilter::TF_Trilinear) {
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

LoadedTextureResult* loadTextureAnyThreadPart(
    const CesiumGltf::Model& model,
    const CesiumGltf::Texture& texture) {

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
    if (!image.mipPositions.empty()) {
      filter = TextureFilter::TF_Trilinear;
    } else if (!pSampler->minFilter && !pSampler->magFilter) {
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

bool loadTextureGameThreadPart(LoadedTextureResult* pHalfLoadedTexture) {
  if (!pHalfLoadedTexture) {
    return false;
  }

  UTexture2D*& pTexture = pHalfLoadedTexture->pTexture;

  if (!pTexture) {
    pTexture = NewObject<UTexture2D>(
        GetTransientPackage(),
        NAME_None,
        RF_Transient | RF_DuplicateTransient | RF_TextExportTransient);

#if ENGINE_MAJOR_VERSION >= 5
    pTexture->SetPlatformData(pHalfLoadedTexture->pTextureData);
#else
    pTexture->PlatformData = pHalfLoadedTexture->pTextureData;
#endif
    pTexture->AddressX = pHalfLoadedTexture->addressX;
    pTexture->AddressY = pHalfLoadedTexture->addressY;
    pTexture->Filter = pHalfLoadedTexture->filter;
    pTexture->UpdateResource();
  }

  return true;
}

namespace {

struct EncodedPixelFormat {
  EPixelFormat format;
  size_t pixelSize;
};

// TODO: consider picking better pixel formats when they are available for the
// current platform.
EncodedPixelFormat getPixelFormat(
    ECesiumMetadataPackedGpuType type,
    int64 componentCount,
    bool isNormalized) {

  switch (type) {
  case ECesiumMetadataPackedGpuType::Uint8:
    switch (componentCount) {
    case 1:
      return {isNormalized ? EPixelFormat::PF_R8 : EPixelFormat::PF_R8_UINT, 1};
    case 2:
    case 3:
    case 4:
      return {
          isNormalized ? EPixelFormat::PF_R8G8B8A8
                       : EPixelFormat::PF_R8G8B8A8_UINT,
          4};
    default:
      return {EPixelFormat::PF_Unknown, 0};
    }
  case ECesiumMetadataPackedGpuType::Float:
    switch (componentCount) {
    case 1:
      return {EPixelFormat::PF_R32_FLOAT, 4};
    case 2:
    case 3:
    case 4:
      // Note this is ABGR
      return {EPixelFormat::PF_A32B32G32R32F, 16};
    }
  default:
    return {EPixelFormat::PF_Unknown, 0};
  }
}
} // namespace

EncodedMetadataFeatureTable encodeMetadataFeatureTableAnyThreadPart(
    const FFeatureTableDescription& encodeInstructions,
    const FCesiumMetadataFeatureTable& featureTable) {

  EncodedMetadataFeatureTable encodedFeatureTable;

  int64 featureCount =
      UCesiumMetadataFeatureTableBlueprintLibrary::GetNumberOfFeatures(
          featureTable);

  const TMap<FString, FCesiumMetadataProperty>& properties =
      UCesiumMetadataFeatureTableBlueprintLibrary::GetProperties(featureTable);

  encodedFeatureTable.encodedProperties.Reserve(properties.Num());
  for (const auto& pair : properties) {
    const FCesiumMetadataProperty& property = pair.Value;

    const FPropertyDescription* pExpectedProperty = nullptr;
    for (const FPropertyDescription& expectedProperty :
         encodeInstructions.Properties) {
      if (pair.Key == expectedProperty.Name) {
        pExpectedProperty = &expectedProperty;
        break;
      }
    }

    if (!pExpectedProperty) {
      continue;
    }

    // TODO: should be some user inputed ECesiumMetadataSupportedGpuType that
    // we check against the trueType
    ECesiumMetadataTrueType trueType =
        UCesiumMetadataPropertyBlueprintLibrary::GetTrueType(property);
    bool isArray = trueType == ECesiumMetadataTrueType::Array;
    bool isNormalized =
        UCesiumMetadataPropertyBlueprintLibrary::IsNormalized(property);

    int64 componentCount;
    if (isArray) {
      trueType = UCesiumMetadataPropertyBlueprintLibrary::GetTrueComponentType(
          property);
      componentCount =
          UCesiumMetadataPropertyBlueprintLibrary::GetComponentCount(property);
    } else {
      componentCount = 1;
    }

    int32 expectedComponentCount = 1;
    switch (pExpectedProperty->Type) {
    // case ECesiumPropertyType::Scalar:
    //  expectedComponentCount = 1;
    //  break;
    case ECesiumPropertyType::Vec2:
      expectedComponentCount = 2;
      break;
    case ECesiumPropertyType::Vec3:
      expectedComponentCount = 3;
      break;
    case ECesiumPropertyType::Vec4:
      expectedComponentCount = 4;
    };

    if (expectedComponentCount != componentCount) {
      continue;
    }

    // TODO: should be some user inputed target gpu type, instead of picking
    // the default gpu type for the source type. Warnings should be displayed
    // when the conversion is lossy. We can even attempt to convert strings to
    // numeric gpu types, but the target format should be explicitly set by the
    // user. [Done ?]
    // TODO: Investigate if this commented out functionality is needed at all
    // now, can it be removed.
    /*
    ECesiumMetadataPackedGpuType gpuType =
        CesiumMetadataTrueTypeToDefaultPackedGpuType(trueType);

    if (gpuType == ECesiumMetadataPackedGpuType::None) {
      continue;
    }*/

    // Coerce the true type into the expected gpu component type.
    ECesiumMetadataPackedGpuType gpuType = ECesiumMetadataPackedGpuType::None;
    if (pExpectedProperty->ComponentType ==
        ECesiumPropertyComponentType::Uint8) {
      gpuType = ECesiumMetadataPackedGpuType::Uint8;
    } else /*if (expected type is float)*/ {
      gpuType = ECesiumMetadataPackedGpuType::Float;
    }

    if (pExpectedProperty->Normalized != isNormalized) {
      continue;
    }

    // Only support normalization of uint8 for now
    if (isNormalized && trueType != ECesiumMetadataTrueType::Uint8) {
      continue;
    }

    EncodedPixelFormat encodedFormat =
        getPixelFormat(gpuType, componentCount, isNormalized);

    if (encodedFormat.format == EPixelFormat::PF_Unknown) {
      continue;
    }

    EncodedMetadataProperty& encodedProperty =
        encodedFeatureTable.encodedProperties.Emplace_GetRef();
    encodedProperty.name = "FTB_" + encodeInstructions.Name + "_" + pair.Key;

    int64 propertyArraySize = featureCount * encodedFormat.pixelSize;
    encodedProperty.pTexture = new LoadedTextureResult();
    encodedProperty.pTexture->pTextureData =
        createTexturePlatformData(propertyArraySize, 1, encodedFormat.format);

    encodedProperty.pTexture->addressX = TextureAddress::TA_Clamp;
    encodedProperty.pTexture->addressY = TextureAddress::TA_Clamp;
    encodedProperty.pTexture->filter = TextureFilter::TF_Nearest;

    if (!encodedProperty.pTexture->pTextureData) {
      // TODO: print error?
      break;
    }

    FTexture2DMipMap* pMip = new FTexture2DMipMap();
    encodedProperty.pTexture->pTextureData->Mips.Add(pMip);
    pMip->SizeX = propertyArraySize;
    pMip->SizeY = 1;
    pMip->BulkData.Lock(LOCK_READ_WRITE);

    void* pTextureData = pMip->BulkData.Realloc(propertyArraySize);

    if (isArray) {
      switch (gpuType) {
      case ECesiumMetadataPackedGpuType::Uint8: {
        uint8* pWritePos = reinterpret_cast<uint8*>(pTextureData);
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
      case ECesiumMetadataPackedGpuType::Float: {
        uint8* pWritePos = reinterpret_cast<uint8*>(pTextureData);
        for (int64 i = 0; i < featureCount; ++i) {
          FCesiumMetadataArray arrayProperty =
              UCesiumMetadataPropertyBlueprintLibrary::GetArray(property, i);
          // Floats are encoded backwards (e.g., ABGR)
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
    } else {
      switch (gpuType) {
      case ECesiumMetadataPackedGpuType::Uint8: {
        uint8* pWritePos = reinterpret_cast<uint8*>(pTextureData);
        for (int64 i = 0; i < featureCount; ++i) {
          *pWritePos =
              UCesiumMetadataPropertyBlueprintLibrary::GetByte(property, i);
          ++pWritePos;
        }
      } break;
      case ECesiumMetadataPackedGpuType::Float: {
        float* pWritePosF = reinterpret_cast<float*>(pTextureData);
        for (int64 i = 0; i < featureCount; ++i) {
          *pWritePosF =
              UCesiumMetadataPropertyBlueprintLibrary::GetFloat(property, i);
          ++pWritePosF;
        }
      } break;
      }
    }

    pMip->BulkData.Unlock();
  }

  return encodedFeatureTable;
}

EncodedFeatureTexture encodeFeatureTextureAnyThreadPart(
    TMap<const CesiumGltf::ImageCesium*, LoadedTextureResult*>&
        featureTexturePropertyMap,
    const FString& featureTextureName,
    const FCesiumFeatureTexture& featureTexture) {
  EncodedFeatureTexture encodedFeatureTexture;

  // TODO: only load requested feature texture properties

  const CesiumGltf::FeatureTextureView& featureTextureView =
      featureTexture.getFeatureTextureView();
  const std::unordered_map<std::string, CesiumGltf::FeatureTexturePropertyView>&
      properties = featureTextureView.getProperties();

  encodedFeatureTexture.properties.Reserve(properties.size());

  for (const auto& propertyIt : properties) {
    const CesiumGltf::FeatureTexturePropertyView& featureTexturePropertyView =
        propertyIt.second;
    EncodedFeatureTextureProperty& encodedFeatureTextureProperty =
        encodedFeatureTexture.properties.Emplace_GetRef();

    const CesiumGltf::ImageCesium* pImage =
        featureTexturePropertyView.getImage();

    if (!pImage) {
      continue;
    }

    encodedFeatureTextureProperty.baseName =
        "FTX_" + featureTextureName + "_" +
        UTF8_TO_TCHAR(propertyIt.first.c_str()) + "_";
    encodedFeatureTextureProperty.textureCoordinateIndex =
        featureTexturePropertyView.getTextureCoordinateIndex();

    const CesiumGltf::FeatureTexturePropertyChannelOffsets& channelOffsets =
        featureTexturePropertyView.getChannelOffsets();
    encodedFeatureTextureProperty.channelOffsets[0] = channelOffsets.r;
    encodedFeatureTextureProperty.channelOffsets[1] = channelOffsets.g;
    encodedFeatureTextureProperty.channelOffsets[2] = channelOffsets.b;
    encodedFeatureTextureProperty.channelOffsets[3] = channelOffsets.a;

    LoadedTextureResult** pMappedUnrealImageIt =
        featureTexturePropertyMap.Find(pImage);
    if (pMappedUnrealImageIt) {
      encodedFeatureTextureProperty.pTexture = *pMappedUnrealImageIt;
    } else {
      encodedFeatureTextureProperty.pTexture = new LoadedTextureResult{};
      encodedFeatureTextureProperty.pTexture->pTextureData =
          createTexturePlatformData(
              pImage->width,
              pImage->height,
              // TODO: currently the unnormalized pixels are always in
              // unsigned R8G8B8A8 form, but this does not necessarily need
              // to be the case in the future.
              featureTexturePropertyView.isNormalized()
                  ? EPixelFormat::PF_R8G8B8A8
                  : EPixelFormat::PF_R8G8B8A8_UINT);

      encodedFeatureTextureProperty.pTexture->addressX =
          TextureAddress::TA_Clamp;
      encodedFeatureTextureProperty.pTexture->addressY =
          TextureAddress::TA_Clamp;
      encodedFeatureTextureProperty.pTexture->filter =
          TextureFilter::TF_Nearest;

      if (!encodedFeatureTextureProperty.pTexture->pTextureData) {
        continue;
      }

      FTexture2DMipMap* pMip = new FTexture2DMipMap();
      encodedFeatureTextureProperty.pTexture->pTextureData->Mips.Add(pMip);
      pMip->SizeX = pImage->width;
      pMip->SizeY = pImage->height;
      pMip->BulkData.Lock(LOCK_READ_WRITE);

      void* pTextureData = pMip->BulkData.Realloc(pImage->pixelData.size());

      FMemory::Memcpy(
          pTextureData,
          pImage->pixelData.data(),
          pImage->pixelData.size());

      pMip->BulkData.Unlock();
    }
  }

  return encodedFeatureTexture;
}

EncodedMetadataPrimitive encodeMetadataPrimitiveAnyThreadPart(
    const UCesiumEncodedMetadataComponent& encodeInstructions,
    const FCesiumMetadataPrimitive& primitive) {
  EncodedMetadataPrimitive result;

  const TArray<FCesiumFeatureIDTexture>& featureIdTextures =
      UCesiumMetadataPrimitiveBlueprintLibrary::GetFeatureIDTextures(primitive);
  const TArray<FCesiumVertexMetadata>& vertexFeatures =
      UCesiumMetadataPrimitiveBlueprintLibrary::GetVertexFeatures(primitive);

  const TArray<FString>& featureTextureNames =
      UCesiumMetadataPrimitiveBlueprintLibrary::GetFeatureTextureNames(
          primitive);
  result.featureTextureNames.Reserve(featureTextureNames.Num());

  for (const FFeatureTextureDescription& expectedFeatureTexture :
       encodeInstructions.FeatureTextures) {
    for (const FString& featureTextureName : featureTextureNames) {
      if (featureTextureName == expectedFeatureTexture.Name) {
        result.featureTextureNames.Add(featureTextureName);
        break;
      }
    }
  }

  std::unordered_map<const CesiumGltf::ImageCesium*, LoadedTextureResult*>
      featureIdTextureMap;
  featureIdTextureMap.reserve(featureIdTextures.Num());

  result.encodedFeatureIdTextures.Reserve(featureIdTextures.Num());
  result.encodedVertexMetadata.Reserve(vertexFeatures.Num());

  // Imposed implementation limitation: Assume only upto one feature id texture
  // or attribute corresponds to each feature table.
  for (const FFeatureTableDescription& expectedFeatureTable :
       encodeInstructions.FeatureTables) {
    if (expectedFeatureTable.AccessType ==
        ECesiumFeatureTableAccessType::Texture) {
      for (const FCesiumFeatureIDTexture& featureIdTexture :
           featureIdTextures) {
        const FString& featureTableName =
            UCesiumFeatureIDTextureBlueprintLibrary::GetFeatureTableName(
                featureIdTexture);

        if (expectedFeatureTable.Name == featureTableName) {

          const CesiumGltf::FeatureIDTextureView& featureIdTextureView =
              featureIdTexture.getFeatureIdTextureView();
          const CesiumGltf::ImageCesium* pFeatureIdImage =
              featureIdTextureView.getImage();

          if (!pFeatureIdImage) {
            break;
          }

          EncodedFeatureIdTexture& encodedFeatureIdTexture =
              result.encodedFeatureIdTextures.Emplace_GetRef();

          encodedFeatureIdTexture.baseName = "FIT_" + featureTableName + "_";
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
                    // TODO: currently this is always the case, but doesn't have
                    // to be
                    EPixelFormat::PF_R8G8B8A8_UINT);

            encodedFeatureIdTexture.pTexture->addressX =
                TextureAddress::TA_Clamp;
            encodedFeatureIdTexture.pTexture->addressY =
                TextureAddress::TA_Clamp;
            encodedFeatureIdTexture.pTexture->filter =
                TextureFilter::TF_Nearest;

            if (!encodedFeatureIdTexture.pTexture->pTextureData) {
              break;
            }

            FTexture2DMipMap* pMip = new FTexture2DMipMap();
            encodedFeatureIdTexture.pTexture->pTextureData->Mips.Add(pMip);
            pMip->SizeX = pFeatureIdImage->width;
            pMip->SizeY = pFeatureIdImage->height;
            pMip->BulkData.Lock(LOCK_READ_WRITE);

            void* pTextureData =
                pMip->BulkData.Realloc(pFeatureIdImage->pixelData.size());

            FMemory::Memcpy(
                pTextureData,
                pFeatureIdImage->pixelData.data(),
                pFeatureIdImage->pixelData.size());

            pMip->BulkData.Unlock();
          }

          encodedFeatureIdTexture.featureTableName = featureTableName;

          break;
        }
      }
    } else if (
        expectedFeatureTable.AccessType ==
        ECesiumFeatureTableAccessType::Attribute) {
      for (size_t i = 0; i < vertexFeatures.Num(); ++i) {
        const FCesiumVertexMetadata& vertexFeature = vertexFeatures[i];
        const FString& featureTableName =
            UCesiumVertexMetadataBlueprintLibrary::GetFeatureTableName(
                vertexFeature);

        if (expectedFeatureTable.Name == featureTableName) {
          EncodedVertexMetadata& encodedVertexFeature =
              result.encodedVertexMetadata.Emplace_GetRef();

          encodedVertexFeature.name = "FA_" + featureTableName;
          encodedVertexFeature.featureTableName = featureTableName;
          encodedVertexFeature.index = static_cast<int32>(i);

          break;
        }
      }
    }
  }

  return result;
}

EncodedMetadata encodeMetadataAnyThreadPart(
    const UCesiumEncodedMetadataComponent& encodeInstructions,
    const FCesiumMetadataModel& metadata) {

  EncodedMetadata result;

  const TMap<FString, FCesiumMetadataFeatureTable>& featureTables =
      UCesiumMetadataModelBlueprintLibrary::GetFeatureTables(metadata);
  result.encodedFeatureTables.Reserve(featureTables.Num());
  for (const auto& featureTableIt : featureTables) {
    for (const FFeatureTableDescription& expectedFeatureTable :
         encodeInstructions.FeatureTables) {
      if (expectedFeatureTable.Name == featureTableIt.Key) {
        result.encodedFeatureTables.Emplace(
            featureTableIt.Key,
            encodeMetadataFeatureTableAnyThreadPart(
                expectedFeatureTable,
                featureTableIt.Value));
        break;
      }
    }
  }

  const TMap<FString, FCesiumFeatureTexture>& featureTextures =
      UCesiumMetadataModelBlueprintLibrary::GetFeatureTextures(metadata);
  result.encodedFeatureTextures.Reserve(featureTextures.Num());
  TMap<const CesiumGltf::ImageCesium*, LoadedTextureResult*>
      featureTexturePropertyMap;
  featureTexturePropertyMap.Reserve(featureTextures.Num());
  for (const auto& featureTextureIt : featureTextures) {
    for (const FFeatureTextureDescription& expectedFeatureTexture :
         encodeInstructions.FeatureTextures) {
      if (expectedFeatureTexture.Name == featureTextureIt.Key) {
        result.encodedFeatureTextures.Emplace(
            featureTextureIt.Key,
            encodeFeatureTextureAnyThreadPart(
                featureTexturePropertyMap,
                featureTextureIt.Key,
                featureTextureIt.Value));
        break;
      }
    }
  }

  return result;
}

bool encodeMetadataFeatureTableGameThreadPart(
    EncodedMetadataFeatureTable& encodedFeatureTable) {
  bool success = true;

  for (EncodedMetadataProperty& encodedProperty :
       encodedFeatureTable.encodedProperties) {
    success |= loadTextureGameThreadPart(encodedProperty.pTexture);
    // TODO: check if this is the safest way
    encodedProperty.pTexture->pTexture->AddToRoot();
  }

  return success;
}

bool encodeFeatureTextureGameThreadPart(
    TArray<LoadedTextureResult*>& uniqueTextures,
    EncodedFeatureTexture& encodedFeatureTexture) {
  bool success = true;

  for (EncodedFeatureTextureProperty& property :
       encodedFeatureTexture.properties) {
    bool found = false;
    for (LoadedTextureResult* uniqueTexture : uniqueTextures) {
      if (property.pTexture == uniqueTexture) {
        found = true;
        break;
      }
    }

    if (!found) {
      success |= loadTextureGameThreadPart(property.pTexture);
      // TODO: check if this is the safest way
      property.pTexture->pTexture->AddToRoot();
      uniqueTextures.Emplace(property.pTexture);
    }
  }

  return success;
}

bool encodeMetadataPrimitiveGameThreadPart(
    EncodedMetadataPrimitive& encodedPrimitive) {
  bool success = true;

  TArray<const LoadedTextureResult*> uniqueFeatureIdImages;
  uniqueFeatureIdImages.Reserve(
      encodedPrimitive.encodedFeatureIdTextures.Num());

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
      success |= loadTextureGameThreadPart(encodedFeatureIdTexture.pTexture);
      uniqueFeatureIdImages.Emplace(encodedFeatureIdTexture.pTexture);
    }
  }

  return success;
}

bool encodeMetadataGameThreadPart(EncodedMetadata& encodedMetadata) {
  bool success = true;

  TArray<LoadedTextureResult*> uniqueTextures;
  uniqueTextures.Reserve(encodedMetadata.encodedFeatureTextures.Num());
  for (auto& encodedFeatureTextureIt : encodedMetadata.encodedFeatureTextures) {
    success |= encodeFeatureTextureGameThreadPart(
        uniqueTextures,
        encodedFeatureTextureIt.Value);
  }

  for (auto& encodedFeatureTableIt : encodedMetadata.encodedFeatureTables) {
    success |=
        encodeMetadataFeatureTableGameThreadPart(encodedFeatureTableIt.Value);
  }

  return success;
}

void destroyEncodedMetadataPrimitive(
    EncodedMetadataPrimitive& encodedPrimitive) {
  for (EncodedFeatureIdTexture& encodedFeatureIdTexture :
       encodedPrimitive.encodedFeatureIdTextures) {

    if (encodedFeatureIdTexture.pTexture->pTexture) {
      CesiumLifetime::destroy(encodedFeatureIdTexture.pTexture->pTexture);
      encodedFeatureIdTexture.pTexture->pTexture = nullptr;
    }
  }
}

void destroyEncodedMetadata(EncodedMetadata& encodedMetadata) {

  // Destroy encoded feature tables.
  for (auto& encodedFeatureTableIt : encodedMetadata.encodedFeatureTables) {
    for (EncodedMetadataProperty& encodedProperty :
         encodedFeatureTableIt.Value.encodedProperties) {
      encodedProperty.pTexture->pTexture->RemoveFromRoot();
      CesiumLifetime::destroy(encodedProperty.pTexture->pTexture);
      encodedProperty.pTexture->pTexture = nullptr;
    }
  }

  // Destroy encoded feature textures.
  for (auto& encodedFeatureTextureIt : encodedMetadata.encodedFeatureTextures) {
    for (EncodedFeatureTextureProperty& encodedFeatureTextureProperty :
         encodedFeatureTextureIt.Value.properties) {
      if (encodedFeatureTextureProperty.pTexture->pTexture) {
        CesiumLifetime::destroy(
            encodedFeatureTextureProperty.pTexture->pTexture);
        encodedFeatureTextureProperty.pTexture->pTexture->RemoveFromRoot();
        encodedFeatureTextureProperty.pTexture->pTexture = nullptr;
      }
    }
  }
}
} // namespace CesiumTextureUtility
