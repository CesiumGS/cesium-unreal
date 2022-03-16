
#include "CesiumEncodedMetadataUtility.h"
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
#include <CesiumGltf/FeatureIDTextureView.h>
#include <CesiumGltf/FeatureTexturePropertyView.h>
#include <CesiumGltf/FeatureTextureView.h>
#include <CesiumUtility/Tracing.h>
#include <unordered_map>

using namespace CesiumTextureUtility;

namespace CesiumEncodedMetadataUtility {

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

    CESIUM_TRACE(
        TCHAR_TO_UTF8(*("Encode Property Array: " + encodeInstructions.Name)));

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
    const FFeatureTextureDescription& featureTextureDescription,
    const FString& featureTextureName,
    const FCesiumFeatureTexture& featureTexture) {
  EncodedFeatureTexture encodedFeatureTexture;

  const CesiumGltf::FeatureTextureView& featureTextureView =
      featureTexture.getFeatureTextureView();
  const std::unordered_map<std::string, CesiumGltf::FeatureTexturePropertyView>&
      properties = featureTextureView.getProperties();

  encodedFeatureTexture.properties.Reserve(properties.size());

  for (const auto& propertyIt : properties) {
    for (const FFeatureTexturePropertyDescription& propertyDescription :
         featureTextureDescription.Properties) {
      if (propertyDescription.Name == UTF8_TO_TCHAR(propertyIt.first.c_str())) {
        const CesiumGltf::FeatureTexturePropertyView&
            featureTexturePropertyView = propertyIt.second;

        const CesiumGltf::ImageCesium* pImage =
            featureTexturePropertyView.getImage();

        if (!pImage) {
          break;
        }

        int32 expectedComponentCount = 1;
        switch (propertyDescription.Type) {
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

        if (expectedComponentCount != propertyIt.second.getComponentCount() ||
            propertyDescription.Normalized !=
                propertyIt.second.isNormalized() ||
            propertyDescription.Swizzle !=
                UTF8_TO_TCHAR(propertyIt.second.getSwizzle().c_str())) {
          break;
        }

        CESIUM_TRACE(TCHAR_TO_UTF8(
            *("Encode Feature Texture Property: " + propertyDescription.Name)));

        EncodedFeatureTextureProperty& encodedFeatureTextureProperty =
            encodedFeatureTexture.properties.Emplace_GetRef();

        encodedFeatureTextureProperty.baseName =
            "FTX_" + featureTextureName + "_" + propertyDescription.Name + "_";
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
            break;
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

        break;
      }
    }
  }

  return encodedFeatureTexture;
}

EncodedMetadataPrimitive encodeMetadataPrimitiveAnyThreadPart(
    const UCesiumEncodedMetadataComponent& encodeInstructions,
    const FCesiumMetadataPrimitive& primitive) {

  CESIUM_TRACE("Encode Metadata Primitive");

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

          CESIUM_TRACE("Encode Feature Id Texture");

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

  CESIUM_TRACE("Encode Metadata Model");

  EncodedMetadata result;

  const TMap<FString, FCesiumMetadataFeatureTable>& featureTables =
      UCesiumMetadataModelBlueprintLibrary::GetFeatureTables(metadata);
  result.encodedFeatureTables.Reserve(featureTables.Num());
  for (const auto& featureTableIt : featureTables) {
    for (const FFeatureTableDescription& expectedFeatureTable :
         encodeInstructions.FeatureTables) {
      if (expectedFeatureTable.Name == featureTableIt.Key) {
        CESIUM_TRACE(
            TCHAR_TO_UTF8(*("Encode Feature Table: " + featureTableIt.Key)));

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
        CESIUM_TRACE(TCHAR_TO_UTF8(
            *("Encode Feature Texture: " + featureTextureIt.Key)));

        result.encodedFeatureTextures.Emplace(
            featureTextureIt.Key,
            encodeFeatureTextureAnyThreadPart(
                featureTexturePropertyMap,
                expectedFeatureTexture,
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
        encodedFeatureTextureProperty.pTexture->pTexture->RemoveFromRoot();
        CesiumLifetime::destroy(
            encodedFeatureTextureProperty.pTexture->pTexture);
        encodedFeatureTextureProperty.pTexture->pTexture = nullptr;
      }
    }
  }
}
} // namespace CesiumEncodedMetadataUtility
