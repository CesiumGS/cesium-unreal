// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumEncodedMetadataUtility.h"
#include "CesiumEncodedMetadataComponent.h"
#include "CesiumFeatureIdAttribute.h"
#include "CesiumFeatureIdTexture.h"
#include "CesiumLifetime.h"
#include "CesiumMetadataPrimitive.h"
#include "CesiumModelMetadata.h"
#include "CesiumPropertyArray.h"
#include "CesiumPropertyArrayBlueprintLibrary.h"
#include "CesiumPropertyTable.h"
#include "CesiumPropertyTableProperty.h"
#include "CesiumPropertyTexture.h"
#include "CesiumRuntime.h"
#include "Containers/Map.h"
#include "PixelFormat.h"
#include "TextureResource.h"
#include "UnrealMetadataConversions.h"
#include <CesiumGltf/FeatureIdTextureView.h>
#include <CesiumGltf/PropertyTexturePropertyView.h>
#include <CesiumGltf/PropertyTextureView.h>
#include <CesiumUtility/Tracing.h>
#include <unordered_map>

using namespace CesiumTextureUtility;

PRAGMA_DISABLE_DEPRECATION_WARNINGS

namespace CesiumEncodedMetadataUtility {

namespace {

struct EncodedPixelFormat {
  EPixelFormat format;
  int32_t bytesPerChannel;
  int32_t channels;
};

// TODO: consider picking better pixel formats when they are available for the
// current platform.
EncodedPixelFormat getPixelFormat(
    ECesiumMetadataPackedGpuType_DEPRECATED type,
    int64 componentCount,
    bool isNormalized) {

  switch (type) {
  case ECesiumMetadataPackedGpuType_DEPRECATED::Uint8_DEPRECATED:
    switch (componentCount) {
    case 1:
      return {
          isNormalized ? EPixelFormat::PF_R8 : EPixelFormat::PF_R8_UINT,
          1,
          1};
    case 2:
    case 3:
    case 4:
      return {
          isNormalized ? EPixelFormat::PF_R8G8B8A8
                       : EPixelFormat::PF_R8G8B8A8_UINT,
          1,
          4};
    default:
      return {EPixelFormat::PF_Unknown, 0};
    }
  case ECesiumMetadataPackedGpuType_DEPRECATED::Float_DEPRECATED:
    switch (componentCount) {
    case 1:
      return {EPixelFormat::PF_R32_FLOAT, 4, 1};
    case 2:
    case 3:
    case 4:
      // Note this is ABGR
      return {EPixelFormat::PF_A32B32G32R32F, 4, 4};
    }
  default:
    return {EPixelFormat::PF_Unknown, 0, 0};
  }
}
} // namespace

EncodedMetadataFeatureTable encodeMetadataFeatureTableAnyThreadPart(
    const FFeatureTableDescription& featureTableDescription,
    const FCesiumPropertyTable& featureTable) {

  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::EncodeFeatureTable)

  EncodedMetadataFeatureTable encodedFeatureTable;

  int64 featureCount =
      UCesiumPropertyTableBlueprintLibrary::GetPropertyTableCount(featureTable);

  const TMap<FString, FCesiumPropertyTableProperty>& properties =
      UCesiumPropertyTableBlueprintLibrary::GetProperties(featureTable);

  encodedFeatureTable.encodedProperties.Reserve(properties.Num());
  for (const auto& pair : properties) {
    const FCesiumPropertyTableProperty& property = pair.Value;

    const FPropertyDescription* pExpectedProperty =
        featureTableDescription.Properties.FindByPredicate(
            [&key = pair.Key](const FPropertyDescription& expectedProperty) {
              return key == expectedProperty.Name;
            });

    if (!pExpectedProperty) {
      continue;
    }

    FCesiumMetadataValueType trueType =
        UCesiumPropertyTablePropertyBlueprintLibrary::GetValueType(property);
    bool isArray = trueType.bIsArray;
    bool isNormalized =
        UCesiumPropertyTablePropertyBlueprintLibrary::IsNormalized(property);

    int64 componentCount;
    if (isArray) {
      componentCount =
          UCesiumPropertyTablePropertyBlueprintLibrary::GetArraySize(property);
    } else {
      componentCount = 1;
    }

    int32 expectedComponentCount = 1;
    switch (pExpectedProperty->Type) {
    // case ECesiumPropertyType::Scalar:
    //  expectedComponentCount = 1;
    //  break;
    case ECesiumPropertyType_DEPRECATED::Vec2_DEPRECATED:
      expectedComponentCount = 2;
      break;
    case ECesiumPropertyType_DEPRECATED::Vec3_DEPRECATED:
      expectedComponentCount = 3;
      break;
    case ECesiumPropertyType_DEPRECATED::Vec4_DEPRECATED:
      expectedComponentCount = 4;
    };

    if (expectedComponentCount != componentCount) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT("Unexpected component count in feature table property."));
      continue;
    }

    // Coerce the true type into the expected gpu component type.
    ECesiumMetadataPackedGpuType_DEPRECATED gpuType =
        ECesiumMetadataPackedGpuType_DEPRECATED::None_DEPRECATED;
    if (pExpectedProperty->ComponentType ==
        ECesiumPropertyComponentType_DEPRECATED::Uint8_DEPRECATED) {
      gpuType = ECesiumMetadataPackedGpuType_DEPRECATED::Uint8_DEPRECATED;
    } else /*if (expected type is float)*/ {
      gpuType = ECesiumMetadataPackedGpuType_DEPRECATED::Float_DEPRECATED;
    }

    if (pExpectedProperty->Normalized != isNormalized) {
      if (isNormalized) {
        UE_LOG(
            LogCesium,
            Warning,
            TEXT("Unexpected normalization in feature table property."));
      } else {
        UE_LOG(
            LogCesium,
            Warning,
            TEXT("Feature table property not normalized as expected"));
      }
      continue;
    }

    // Only support normalization of uint8 for now
    if (isNormalized &&
        trueType.ComponentType != ECesiumMetadataComponentType::Uint8) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT(
              "Feature table property has unexpected type for normalization, only normalization of Uint8 is supported."));
      continue;
    }

    EncodedPixelFormat encodedFormat =
        getPixelFormat(gpuType, componentCount, isNormalized);

    if (encodedFormat.format == EPixelFormat::PF_Unknown) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT(
              "Unable to determine a suitable GPU format for this feature table property."));
      continue;
    }

    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::EncodePropertyArray)

    EncodedMetadataProperty& encodedProperty =
        encodedFeatureTable.encodedProperties.Emplace_GetRef();
    encodedProperty.name =
        "FTB_" + featureTableDescription.Name + "_" + pair.Key;

    int64 floorSqrtFeatureCount = glm::sqrt(featureCount);
    int64 ceilSqrtFeatureCount =
        (floorSqrtFeatureCount * floorSqrtFeatureCount == featureCount)
            ? floorSqrtFeatureCount
            : (floorSqrtFeatureCount + 1);

    CesiumUtility::IntrusivePointer<CesiumGltf::ImageCesium> pImage =
        new CesiumGltf::ImageCesium();
    pImage->bytesPerChannel = encodedFormat.bytesPerChannel;
    pImage->channels = encodedFormat.channels;
    pImage->compressedPixelFormat = CesiumGltf::GpuCompressedPixelFormat::NONE;
    pImage->height = pImage->width = ceilSqrtFeatureCount;
    pImage->pixelData.resize(size_t(
        pImage->width * pImage->height * pImage->channels *
        pImage->bytesPerChannel));

    if (isArray) {
      switch (gpuType) {
      case ECesiumMetadataPackedGpuType_DEPRECATED::Uint8_DEPRECATED: {
        uint8* pWritePos = reinterpret_cast<uint8*>(pImage->pixelData.data());
        int64_t pixelSize =
            encodedFormat.channels * encodedFormat.bytesPerChannel;
        for (int64 i = 0; i < featureCount; ++i) {
          FCesiumPropertyArray arrayProperty =
              UCesiumPropertyTablePropertyBlueprintLibrary::GetArray(
                  property,
                  i);
          for (int64 j = 0; j < componentCount; ++j) {
            *(pWritePos + j) =
                UCesiumPropertyArrayBlueprintLibrary::GetByte(arrayProperty, j);
          }
          pWritePos += pixelSize;
        }
      } break;
      case ECesiumMetadataPackedGpuType_DEPRECATED::Float_DEPRECATED: {
        uint8* pWritePos = reinterpret_cast<uint8*>(pImage->pixelData.data());
        int64_t pixelSize =
            encodedFormat.channels * encodedFormat.bytesPerChannel;
        for (int64 i = 0; i < featureCount; ++i) {
          FCesiumPropertyArray arrayProperty =
              UCesiumPropertyTablePropertyBlueprintLibrary::GetArray(
                  property,
                  i);
          // Floats are encoded backwards (e.g., ABGR)
          float* pWritePosF =
              reinterpret_cast<float*>(pWritePos + pixelSize) - 1;
          for (int64 j = 0; j < componentCount; ++j) {
            *pWritePosF = UCesiumPropertyArrayBlueprintLibrary::GetFloat(
                arrayProperty,
                j);
            --pWritePosF;
          }
          pWritePos += pixelSize;
        }
      } break;
      }
    } else {
      switch (gpuType) {
      case ECesiumMetadataPackedGpuType_DEPRECATED::Uint8_DEPRECATED: {
        uint8* pWritePos = reinterpret_cast<uint8*>(pImage->pixelData.data());
        for (int64 i = 0; i < featureCount; ++i) {
          *pWritePos = UCesiumPropertyTablePropertyBlueprintLibrary::GetByte(
              property,
              i);
          ++pWritePos;
        }
      } break;
      case ECesiumMetadataPackedGpuType_DEPRECATED::Float_DEPRECATED: {
        float* pWritePosF = reinterpret_cast<float*>(pImage->pixelData.data());
        for (int64 i = 0; i < featureCount; ++i) {
          *pWritePosF = UCesiumPropertyTablePropertyBlueprintLibrary::GetFloat(
              property,
              i);
          ++pWritePosF;
        }
      } break;
      }
    }

    encodedProperty.pTexture = loadTextureAnyThreadPart(
        *pImage,
        TextureAddress::TA_Clamp,
        TextureAddress::TA_Clamp,
        TextureFilter::TF_Nearest,
        false,
        TEXTUREGROUP_8BitData,
        false,
        encodedFormat.format);
  }

  return encodedFeatureTable;
}

EncodedFeatureTexture encodeFeatureTextureAnyThreadPart(
    TMap<const CesiumGltf::ImageCesium*, TWeakPtr<LoadedTextureResult>>&
        featureTexturePropertyMap,
    const FFeatureTextureDescription& featureTextureDescription,
    const FString& featureTextureName,
    const FCesiumPropertyTexture& featureTexture) {

  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::EncodeFeatureTexture)

  EncodedFeatureTexture encodedFeatureTexture;

  const auto& properties =
      UCesiumPropertyTextureBlueprintLibrary::GetProperties(featureTexture);
  encodedFeatureTexture.properties.Reserve(properties.Num());

  for (const auto& propertyIt : properties) {
    const FFeatureTexturePropertyDescription* pPropertyDescription =
        featureTextureDescription.Properties.FindByPredicate(
            [propertyName = propertyIt.Key](
                const FFeatureTexturePropertyDescription& expectedProperty) {
              return propertyName == expectedProperty.Name;
            });

    if (!pPropertyDescription) {
      continue;
    }

    const FCesiumPropertyTextureProperty& featureTextureProperty =
        propertyIt.Value;

    const CesiumGltf::ImageCesium* pImage = featureTextureProperty.getImage();

    if (!pImage) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT("This feature texture property does not have a valid image."));
      continue;
    }

    int32 expectedComponentCount = 0;
    switch (pPropertyDescription->Type) {
    case ECesiumPropertyType_DEPRECATED::Scalar_DEPRECATED:
      expectedComponentCount = 1;
      break;
    case ECesiumPropertyType_DEPRECATED::Vec2_DEPRECATED:
      expectedComponentCount = 2;
      break;
    case ECesiumPropertyType_DEPRECATED::Vec3_DEPRECATED:
      expectedComponentCount = 3;
      break;
    case ECesiumPropertyType_DEPRECATED::Vec4_DEPRECATED:
      expectedComponentCount = 4;
      break;
    };

    int32 actualComponentCount = 0;
    FCesiumMetadataValueType valueType =
        UCesiumPropertyTexturePropertyBlueprintLibrary::GetValueType(
            featureTextureProperty);
    switch (valueType.Type) {
    case ECesiumMetadataType::Scalar:
      actualComponentCount = 1;
      break;
    case ECesiumMetadataType::Vec2:
      actualComponentCount = 2;
      break;
    case ECesiumMetadataType::Vec3:
      actualComponentCount = 3;
      break;
    case ECesiumMetadataType::Vec4:
      actualComponentCount = 4;
      break;
    }

    if (expectedComponentCount != actualComponentCount) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT(
              "This feature texture property does not have the expected component count"));
      continue;
    }

    bool isNormalized =
        UCesiumPropertyTexturePropertyBlueprintLibrary::IsNormalized(
            featureTextureProperty);
    if (pPropertyDescription->Normalized != isNormalized) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT(
              "This feature texture property does not have the expected normalization."));
      continue;
    }

    TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::EncodeFeatureTextureProperty)

    EncodedFeatureTextureProperty& encodedFeatureTextureProperty =
        encodedFeatureTexture.properties.Emplace_GetRef();

    encodedFeatureTextureProperty.baseName =
        "FTX_" + featureTextureName + "_" + pPropertyDescription->Name + "_";
    encodedFeatureTextureProperty.textureCoordinateAttributeId =
        featureTextureProperty.getTexCoordSetIndex();

    const auto& channels =
        UCesiumPropertyTexturePropertyBlueprintLibrary::GetChannels(
            featureTextureProperty);
    encodedFeatureTextureProperty.channelOffsets[0] = channels[0];
    encodedFeatureTextureProperty.channelOffsets[1] = channels[1];
    encodedFeatureTextureProperty.channelOffsets[2] = channels[2];
    encodedFeatureTextureProperty.channelOffsets[3] = channels[3];

    TWeakPtr<LoadedTextureResult>* pMappedUnrealImageIt =
        featureTexturePropertyMap.Find(pImage);
    if (pMappedUnrealImageIt) {
      encodedFeatureTextureProperty.pTexture = pMappedUnrealImageIt->Pin();
    } else {
      CesiumUtility::IntrusivePointer<CesiumGltf::ImageCesium> pImageCopy =
          new CesiumGltf::ImageCesium(*pImage);
      encodedFeatureTextureProperty.pTexture =
          MakeShared<LoadedTextureResult>(std::move(*loadTextureAnyThreadPart(
              *pImageCopy,
              TextureAddress::TA_Clamp,
              TextureAddress::TA_Clamp,
              TextureFilter::TF_Nearest,
              false,
              TEXTUREGROUP_8BitData,
              false,
              // TODO : currently the unnormalized pixels are always in unsigned
              // R8G8B8A8 form, but this does not necessarily need to be the
              // case in the future.
              isNormalized ? EPixelFormat::PF_R8G8B8A8
                           : EPixelFormat::PF_R8G8B8A8_UINT)));
      featureTexturePropertyMap.Emplace(
          pImage,
          encodedFeatureTextureProperty.pTexture);
    }
  }

  return encodedFeatureTexture;
}

EncodedMetadataPrimitive encodeMetadataPrimitiveAnyThreadPart(
    const FMetadataDescription& metadataDescription,
    const FCesiumMetadataPrimitive& primitive) {

  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::EncodeMetadataPrimitive)

  EncodedMetadataPrimitive result;

  const TArray<FCesiumFeatureIdTexture>& featureIdTextures =
      UCesiumMetadataPrimitiveBlueprintLibrary::GetFeatureIdTextures(primitive);
  const TArray<FCesiumFeatureIdAttribute>& featureIdAttributes =
      UCesiumMetadataPrimitiveBlueprintLibrary::GetFeatureIdAttributes(
          primitive);

  const TArray<FString>& featureTextureNames =
      UCesiumMetadataPrimitiveBlueprintLibrary::GetFeatureTextureNames(
          primitive);
  result.featureTextureNames.Reserve(featureTextureNames.Num());

  for (const FFeatureTextureDescription& expectedFeatureTexture :
       metadataDescription.FeatureTextures) {
    if (featureTextureNames.Find(expectedFeatureTexture.Name) != INDEX_NONE) {
      result.featureTextureNames.Add(expectedFeatureTexture.Name);
    }
  }

  TMap<const CesiumGltf::ImageCesium*, TWeakPtr<LoadedTextureResult>>
      featureIdTextureMap;
  featureIdTextureMap.Reserve(featureIdTextures.Num());

  result.encodedFeatureIdTextures.Reserve(featureIdTextures.Num());
  result.encodedFeatureIdAttributes.Reserve(featureIdAttributes.Num());

  // Imposed implementation limitation: Assume only upto one feature id texture
  // or attribute corresponds to each feature table.
  for (const FFeatureTableDescription& expectedFeatureTable :
       metadataDescription.FeatureTables) {
    const FString& featureTableName = expectedFeatureTable.Name;

    if (expectedFeatureTable.AccessType ==
        ECesiumFeatureTableAccessType_DEPRECATED::Texture_DEPRECATED) {

      const FCesiumFeatureIdTexture* pFeatureIdTexture =
          featureIdTextures.FindByPredicate([&featureTableName](
                                                const FCesiumFeatureIdTexture&
                                                    featureIdTexture) {
            return featureTableName ==
                   UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureTableName(
                       featureIdTexture);
          });

      if (pFeatureIdTexture) {
        const CesiumGltf::FeatureIdTextureView& featureIdTextureView =
            pFeatureIdTexture->getFeatureIdTextureView();
        const CesiumGltf::ImageCesium* pFeatureIdImage =
            featureIdTextureView.getImage();

        if (!pFeatureIdImage) {
          UE_LOG(
              LogCesium,
              Warning,
              TEXT("Feature id texture missing valid image."));
          continue;
        }

        TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::EncodeFeatureIdTexture)

        EncodedFeatureIdTexture& encodedFeatureIdTexture =
            result.encodedFeatureIdTextures.Emplace_GetRef();

        const auto channels = featureIdTextureView.getChannels();
        encodedFeatureIdTexture.baseName = "FIT_" + featureTableName + "_";
        encodedFeatureIdTexture.channel = channels.size() ? channels[0] : 0;
        encodedFeatureIdTexture.textureCoordinateAttributeId =
            featureIdTextureView.getTexCoordSetIndex();

        TWeakPtr<LoadedTextureResult>* pMappedUnrealImageIt =
            featureIdTextureMap.Find(pFeatureIdImage);
        if (pMappedUnrealImageIt) {
          encodedFeatureIdTexture.pTexture = pMappedUnrealImageIt->Pin();
        } else {
          CesiumUtility::IntrusivePointer<CesiumGltf::ImageCesium> pImageCopy =
              new CesiumGltf::ImageCesium(*pFeatureIdImage);
          encodedFeatureIdTexture.pTexture = MakeShared<LoadedTextureResult>(
              std::move(*loadTextureAnyThreadPart(
                  *pImageCopy,
                  TextureAddress::TA_Clamp,
                  TextureAddress::TA_Clamp,
                  TextureFilter::TF_Nearest,
                  false,
                  TEXTUREGROUP_8BitData,
                  false,
                  // TODO: currently this is always the case, but doesn't have
                  // to be
                  EPixelFormat::PF_R8G8B8A8_UINT)));
          featureIdTextureMap.Emplace(
              pFeatureIdImage,
              encodedFeatureIdTexture.pTexture);
        }

        encodedFeatureIdTexture.featureTableName = featureTableName;
      }
    } else if (
        expectedFeatureTable.AccessType ==
        ECesiumFeatureTableAccessType_DEPRECATED::Attribute_DEPRECATED) {
      for (size_t i = 0; i < featureIdAttributes.Num(); ++i) {
        const FCesiumFeatureIdAttribute& featureIdAttribute =
            featureIdAttributes[i];

        if (featureTableName ==
            UCesiumFeatureIdAttributeBlueprintLibrary::GetFeatureTableName(
                featureIdAttribute)) {
          EncodedFeatureIdAttribute& encodedFeatureIdAttribute =
              result.encodedFeatureIdAttributes.Emplace_GetRef();

          encodedFeatureIdAttribute.name = "FA_" + featureTableName;
          encodedFeatureIdAttribute.featureTableName = featureTableName;
          encodedFeatureIdAttribute.index = static_cast<int32>(i);

          break;
        }
      }
    }
  }

  return result;
}

EncodedMetadata encodeMetadataAnyThreadPart(
    const FMetadataDescription& metadataDescription,
    const FCesiumModelMetadata& metadata) {

  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::EncodeMetadataModel)

  EncodedMetadata result;

  const TMap<FString, FCesiumPropertyTable>& featureTables =
      UCesiumModelMetadataBlueprintLibrary::GetFeatureTables(metadata);
  result.encodedFeatureTables.Reserve(featureTables.Num());
  for (const auto& featureTableIt : featureTables) {
    const FString& featureTableName = featureTableIt.Key;

    const FFeatureTableDescription* pExpectedFeatureTable =
        metadataDescription.FeatureTables.FindByPredicate(
            [&featureTableName](
                const FFeatureTableDescription& expectedFeatureTable) {
              return featureTableName == expectedFeatureTable.Name;
            });

    if (pExpectedFeatureTable) {
      TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::EncodeFeatureTable)

      result.encodedFeatureTables.Emplace(
          featureTableName,
          encodeMetadataFeatureTableAnyThreadPart(
              *pExpectedFeatureTable,
              featureTableIt.Value));
    }
  }

  const TMap<FString, FCesiumPropertyTexture>& featureTextures =
      UCesiumModelMetadataBlueprintLibrary::GetFeatureTextures(metadata);
  result.encodedFeatureTextures.Reserve(featureTextures.Num());
  TMap<const CesiumGltf::ImageCesium*, TWeakPtr<LoadedTextureResult>>
      featureTexturePropertyMap;
  featureTexturePropertyMap.Reserve(featureTextures.Num());
  for (const auto& featureTextureIt : featureTextures) {
    const FString& featureTextureName = featureTextureIt.Key;

    const FFeatureTextureDescription* pExpectedFeatureTexture =
        metadataDescription.FeatureTextures.FindByPredicate(
            [&featureTextureName](
                const FFeatureTextureDescription& expectedFeatureTexture) {
              return featureTextureName == expectedFeatureTexture.Name;
            });

    if (pExpectedFeatureTexture) {
      TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::EncodeFeatureTexture)

      result.encodedFeatureTextures.Emplace(
          featureTextureName,
          encodeFeatureTextureAnyThreadPart(
              featureTexturePropertyMap,
              *pExpectedFeatureTexture,
              featureTextureName,
              featureTextureIt.Value));
    }
  }

  return result;
}

bool encodeMetadataFeatureTableGameThreadPart(
    EncodedMetadataFeatureTable& encodedFeatureTable) {
  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::EncodeFeatureTable)

  bool success = true;

  for (EncodedMetadataProperty& encodedProperty :
       encodedFeatureTable.encodedProperties) {
    success &=
        loadTextureGameThreadPart(encodedProperty.pTexture.Get()) != nullptr;
  }

  return success;
}

bool encodeFeatureTextureGameThreadPart(
    TArray<LoadedTextureResult*>& uniqueTextures,
    EncodedFeatureTexture& encodedFeatureTexture) {
  bool success = true;

  for (EncodedFeatureTextureProperty& property :
       encodedFeatureTexture.properties) {
    if (uniqueTextures.Find(property.pTexture.Get()) == INDEX_NONE) {
      success &= loadTextureGameThreadPart(property.pTexture.Get()) != nullptr;
      uniqueTextures.Emplace(property.pTexture.Get());
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
    if (uniqueFeatureIdImages.Find(encodedFeatureIdTexture.pTexture.Get()) ==
        INDEX_NONE) {
      success &= loadTextureGameThreadPart(
                     encodedFeatureIdTexture.pTexture.Get()) != nullptr;
      uniqueFeatureIdImages.Emplace(encodedFeatureIdTexture.pTexture.Get());
    }
  }

  return success;
}

bool encodeMetadataGameThreadPart(EncodedMetadata& encodedMetadata) {
  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::EncodeMetadata)

  bool success = true;

  TArray<LoadedTextureResult*> uniqueTextures;
  uniqueTextures.Reserve(encodedMetadata.encodedFeatureTextures.Num());
  for (auto& encodedFeatureTextureIt : encodedMetadata.encodedFeatureTextures) {
    success &= encodeFeatureTextureGameThreadPart(
        uniqueTextures,
        encodedFeatureTextureIt.Value);
  }

  for (auto& encodedFeatureTableIt : encodedMetadata.encodedFeatureTables) {
    success &=
        encodeMetadataFeatureTableGameThreadPart(encodedFeatureTableIt.Value);
  }

  return success;
}

void destroyEncodedMetadataPrimitive(
    EncodedMetadataPrimitive& encodedPrimitive) {
  for (EncodedFeatureIdTexture& encodedFeatureIdTexture :
       encodedPrimitive.encodedFeatureIdTextures) {

    if (encodedFeatureIdTexture.pTexture) {
      encodedFeatureIdTexture.pTexture->pTexture = nullptr;
    }
  }
}

void destroyEncodedMetadata(EncodedMetadata& encodedMetadata) {

  // Destroy encoded feature tables.
  for (auto& encodedFeatureTableIt : encodedMetadata.encodedFeatureTables) {
    for (EncodedMetadataProperty& encodedProperty :
         encodedFeatureTableIt.Value.encodedProperties) {
      if (encodedProperty.pTexture) {
        encodedProperty.pTexture->pTexture = nullptr;
      }
    }
  }

  // Destroy encoded feature textures.
  for (auto& encodedFeatureTextureIt : encodedMetadata.encodedFeatureTextures) {
    for (EncodedFeatureTextureProperty& encodedFeatureTextureProperty :
         encodedFeatureTextureIt.Value.properties) {
      if (encodedFeatureTextureProperty.pTexture) {
        encodedFeatureTextureProperty.pTexture->pTexture = nullptr;
      }
    }
  }
}

// The result should be a safe hlsl identifier, but any name clashes after
// fixing safety will not be automatically handled.
FString createHlslSafeName(const FString& rawName) {
  static const FString identifierHeadChar =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";
  static const FString identifierTailChar = identifierHeadChar + "0123456789";

  FString safeName = rawName;
  int32 _;
  if (safeName.Len() == 0) {
    return "_";
  } else {
    if (!identifierHeadChar.FindChar(safeName[0], _)) {
      safeName = "_" + safeName;
    }
  }

  for (size_t i = 1; i < safeName.Len(); ++i) {
    if (!identifierTailChar.FindChar(safeName[i], _)) {
      safeName[i] = '_';
    }
  }

  return safeName;
}

} // namespace CesiumEncodedMetadataUtility

PRAGMA_ENABLE_DEPRECATION_WARNINGS
