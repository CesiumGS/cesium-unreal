// Copyright 2020-2021 CesiumGS, Inc. and Contributors

// clang-format off
#include "CesiumEncodedMetadataUtility.h"
#include "CesiumEncodedMetadataComponent.h"
#include "CesiumFeatureIdAttribute.h"
#include "CesiumFeatureIdTexture.h"
#include "CesiumLifetime.h"
#include "CesiumMetadataArray.h"
#include "CesiumMetadataConversions.h"
#include "CesiumModelMetadata.h"
#include "CesiumPrimitiveMetadata.h"
#include "CesiumPropertyTable.h"
#include "CesiumPropertyTexture.h"
#include "CesiumRuntime.h"
#include "Containers/Map.h"
#include "PixelFormat.h"
#include "TextureResource.h"
#include <CesiumGltf/FeatureIdTextureView.h>
#include <CesiumGltf/PropertyTextureView.h>
#include <CesiumUtility/Tracing.h>
#include <glm/gtx/integer.hpp>
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

EncodedPropertyTable encodePropertyTableAnyThreadPart(
    const FFeatureTableDescription& featureTableDescription,
    const FCesiumPropertyTable& propertyTable) {

  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::EncodePropertyTable)

  EncodedPropertyTable encodedPropertyTable;

  int64 propertyTableCount =
      UCesiumPropertyTableBlueprintLibrary::GetPropertyTableSize(propertyTable);

  const TMap<FString, FCesiumPropertyTableProperty>& properties =
      UCesiumPropertyTableBlueprintLibrary::GetProperties(propertyTable);

  encodedPropertyTable.properties.Reserve(properties.Num());
  for (const auto& pair : properties) {
    // const FCesiumPropertyTableProperty& property = pair.Value;

    // const FPropertyDescription* pExpectedProperty =
    //    featureTableDescription.Properties.FindByPredicate(
    //        [&key = pair.Key](const FPropertyDescription& expectedProperty) {
    //          return key == expectedProperty.Name;
    //        });

    // if (!pExpectedProperty) {
    //  continue;
    //}

    // FCesiumMetadataValueType valueType =
    //    UCesiumPropertyTablePropertyBlueprintLibrary::GetValueType(property);
    // bool isArray = valueType.bIsArray;
    // bool isNormalized =
    //    UCesiumPropertyTablePropertyBlueprintLibrary::IsNormalized(property);

    // int64 count = isArray ? UCesiumPropertyTablePropertyBlueprintLibrary::
    //                            GetPropertyArraySize(property)
    //                      : 1;
    // int32 expectedComponentCount = 1;
    // switch (pExpectedProperty->Type) {
    //// case ECesiumPropertyType::Scalar:
    ////  expectedComponentCount = 1;
    ////  break;
    // case ECesiumPropertyType::Vec2:
    //  expectedComponentCount = 2;
    //  break;
    // case ECesiumPropertyType::Vec3:
    //  expectedComponentCount = 3;
    //  break;
    // case ECesiumPropertyType::Vec4:
    //  expectedComponentCount = 4;
    //};

    // if (expectedComponentCount != count) {
    //  UE_LOG(
    //      LogCesium,
    //      Warning,
    //      TEXT("Unexpected component count in feature table property."));
    //  continue;
    //}

    //// Coerce the true type into the expected gpu component type.
    // ECesiumMetadataPackedGpuType gpuType =
    // ECesiumMetadataPackedGpuType::None; if (pExpectedProperty->ComponentType
    // ==
    //    ECesiumPropertyComponentType::Uint8) {
    //  gpuType = ECesiumMetadataPackedGpuType::Uint8;
    //} else /*if (expected type is float)*/ {
    //  gpuType = ECesiumMetadataPackedGpuType::Float;
    //}

    // if (pExpectedProperty->Normalized != isNormalized) {
    //  if (isNormalized) {
    //    UE_LOG(
    //        LogCesium,
    //        Warning,
    //        TEXT("Unexpected normalization in feature table property."));
    //  } else {
    //    UE_LOG(
    //        LogCesium,
    //        Warning,
    //        TEXT("Feature table property not normalized as expected"));
    //  }
    //  continue;
    //}

    //// Only support normalization of uint8 for now
    // if (isNormalized &&
    //    valueType.ComponentType != ECesiumMetadataComponentType::Uint8) {
    //  UE_LOG(
    //      LogCesium,
    //      Warning,
    //      TEXT("Only normalization of Uint8 properties is supported."));
    //  continue;
    //}

    // EncodedPixelFormat encodedFormat =
    //    getPixelFormat(gpuType, count, isNormalized);

    // if (encodedFormat.format == EPixelFormat::PF_Unknown) {
    //  UE_LOG(
    //      LogCesium,
    //      Warning,
    //      TEXT(
    //          "Unable to determine a suitable GPU format for this feature
    //          table property."));
    //  continue;
    //}

    // TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::EncodePropertyArray)

    // EncodedPropertyTableProperty& property =
    //    encodedPropertyTable.properties.Emplace_GetRef();
    // property.name =
    //    "FTB_" + featureTableDescription.Name + "_" + pair.Key;

    // int64 floorSqrtFeatureCount = glm::sqrt(propertyTableCount);
    // int64 ceilSqrtFeatureCount =
    //    (floorSqrtFeatureCount * floorSqrtFeatureCount == propertyTableCount)
    //        ? floorSqrtFeatureCount
    //        : (floorSqrtFeatureCount + 1);
    // encodedProperty.pTexture = MakeUnique<LoadedTextureResult>();
    //// TODO: upgrade to new texture creation path.
    // encodedProperty.pTexture->textureSource = LegacyTextureSource{};
    // encodedProperty.pTexture->pTextureData = createTexturePlatformData(
    //    ceilSqrtFeatureCount,
    //    ceilSqrtFeatureCount,
    //    encodedFormat.format);

    // encodedProperty.pTexture->addressX = TextureAddress::TA_Clamp;
    // encodedProperty.pTexture->addressY = TextureAddress::TA_Clamp;
    // encodedProperty.pTexture->filter = TextureFilter::TF_Nearest;

    // if (!encodedProperty.pTexture->pTextureData) {
    //  UE_LOG(
    //      LogCesium,
    //      Error,
    //      TEXT(
    //          "Error encoding a feature table property. Most likely could not
    //          allocate enough texture memory."));
    //  continue;
    //}

    // FTexture2DMipMap* pMip = new FTexture2DMipMap();
    // encodedProperty.pTexture->pTextureData->Mips.Add(pMip);
    // pMip->SizeX = ceilSqrtFeatureCount;
    // pMip->SizeY = ceilSqrtFeatureCount;
    // pMip->BulkData.Lock(LOCK_READ_WRITE);

    // void* pTextureData = pMip->BulkData.Realloc(
    //    ceilSqrtFeatureCount * ceilSqrtFeatureCount *
    //    encodedFormat.pixelSize);

    // if (isArray) {
    //  switch (gpuType) {
    //  case ECesiumMetadataPackedGpuType::Uint8: {
    //    uint8* pWritePos = reinterpret_cast<uint8*>(pTextureData);
    //    for (int64 i = 0; i < propertyTableCount; ++i) {
    //      FCesiumMetadataArray arrayProperty =
    //          UCesiumMetadataPropertyBlueprintLibrary::GetArray(property, i);
    //      for (int64 j = 0; j < componentCount; ++j) {
    //        *(pWritePos + j) =
    //            UCesiumMetadataArrayBlueprintLibrary::GetByte(arrayProperty,
    //            j);
    //      }
    //      pWritePos += encodedFormat.pixelSize;
    //    }
    //  } break;
    //  case ECesiumMetadataPackedGpuType::Float: {
    //    uint8* pWritePos = reinterpret_cast<uint8*>(pTextureData);
    //    for (int64 i = 0; i < featureCount; ++i) {
    //      FCesiumMetadataArray arrayProperty =
    //          UCesiumMetadataPropertyBlueprintLibrary::GetArray(property, i);
    //      // Floats are encoded backwards (e.g., ABGR)
    //      float* pWritePosF =
    //          reinterpret_cast<float*>(pWritePos + encodedFormat.pixelSize) -
    //          1;
    //      for (int64 j = 0; j < componentCount; ++j) {
    //        *pWritePosF = UCesiumMetadataArrayBlueprintLibrary::GetFloat(
    //            arrayProperty,
    //            j);
    //        --pWritePosF;
    //      }
    //      pWritePos += encodedFormat.pixelSize;
    //    }
    //  } break;
    //  }
    //} else {
    //  switch (gpuType) {
    //  case ECesiumMetadataPackedGpuType::Uint8: {
    //    uint8* pWritePos = reinterpret_cast<uint8*>(pTextureData);
    //    for (int64 i = 0; i < featureCount; ++i) {
    //      *pWritePos =
    //          UCesiumMetadataPropertyBlueprintLibrary::GetByte(property, i);
    //      ++pWritePos;
    //    }
    //  } break;
    //  case ECesiumMetadataPackedGpuType::Float: {
    //    float* pWritePosF = reinterpret_cast<float*>(pTextureData);
    //    for (int64 i = 0; i < featureCount; ++i) {
    //      *pWritePosF =
    //          UCesiumMetadataPropertyBlueprintLibrary::GetFloat(property, i);
    //      ++pWritePosF;
    //    }
    //  } break;
    //  }
    //}

    // pMip->BulkData.Unlock();
  }

  return encodedPropertyTable;
}

EncodedPropertyTexture encodePropertyTextureAnyThreadPart(
    TMap<const CesiumGltf::ImageCesium*, TWeakPtr<LoadedTextureResult>>&
        featureTexturePropertyMap,
    const FFeatureTextureDescription& featureTextureDescription,
    const FString& featureTextureName,
    const FCesiumPropertyTexture& propertyTexture) {

  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::EncodeFeatureTexture)

  EncodedPropertyTexture encodedPropertyTexture;

  const CesiumGltf::PropertyTextureView& propertyTextureView =
      propertyTexture.getPropertyTextureView();
  // const std::unordered_map<std::string,
  // CesiumGltf::FeatureTexturePropertyView>&
  //    properties = featureTextureView.getProperties();

  // encodedFeatureTexture.properties.Reserve(properties.size());

  // for (const auto& propertyIt : properties) {
  //  const FFeatureTexturePropertyDescription* pPropertyDescription =
  //      featureTextureDescription.Properties.FindByPredicate(
  //          [propertyName = UTF8_TO_TCHAR(propertyIt.first.c_str())](
  //              const FFeatureTexturePropertyDescription& expectedProperty) {
  //            return propertyName == expectedProperty.Name;
  //          });

  //  if (!pPropertyDescription) {
  //    continue;
  //  }

  //  const CesiumGltf::FeatureTexturePropertyView& featureTexturePropertyView =
  //      propertyIt.second;

  //  const CesiumGltf::ImageCesium* pImage =
  //      featureTexturePropertyView.getImage();

  //  if (!pImage) {
  //    UE_LOG(
  //        LogCesium,
  //        Warning,
  //        TEXT("This feature texture property does not have a valid image."));
  //    continue;
  //  }

  //  int32 expectedComponentCount = 1;
  //  switch (pPropertyDescription->Type) {
  //  // case ECesiumPropertyType::Scalar:
  //  //  expectedComponentCount = 1;
  //  //  break;
  //  case ECesiumPropertyType::Vec2:
  //    expectedComponentCount = 2;
  //    break;
  //  case ECesiumPropertyType::Vec3:
  //    expectedComponentCount = 3;
  //    break;
  //  case ECesiumPropertyType::Vec4:
  //    expectedComponentCount = 4;
  //  };

  //  if (expectedComponentCount != propertyIt.second.getComponentCount() ||
  //      pPropertyDescription->Normalized != propertyIt.second.isNormalized()
  //      || pPropertyDescription->Swizzle !=
  //          UTF8_TO_TCHAR(propertyIt.second.getSwizzle().c_str())) {
  //    UE_LOG(
  //        LogCesium,
  //        Warning,
  //        TEXT(
  //            "This feature texture property does not have the expected
  //            component count, normalization, or swizzle string."));
  //    continue;
  //  }

  //  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::EncodeFeatureTextureProperty)

  //  EncodedFeatureTextureProperty& encodedFeatureTextureProperty =
  //      encodedFeatureTexture.properties.Emplace_GetRef();

  //  encodedFeatureTextureProperty.baseName =
  //      "FTX_" + featureTextureName + "_" + pPropertyDescription->Name + "_";
  //  encodedFeatureTextureProperty.textureCoordinateAttributeId =
  //      featureTexturePropertyView.getTextureCoordinateAttributeId();

  //  const CesiumGltf::FeatureTexturePropertyChannelOffsets& channelOffsets =
  //      featureTexturePropertyView.getChannelOffsets();
  //  encodedFeatureTextureProperty.channelOffsets[0] = channelOffsets.r;
  //  encodedFeatureTextureProperty.channelOffsets[1] = channelOffsets.g;
  //  encodedFeatureTextureProperty.channelOffsets[2] = channelOffsets.b;
  //  encodedFeatureTextureProperty.channelOffsets[3] = channelOffsets.a;

  //  TWeakPtr<LoadedTextureResult>* pMappedUnrealImageIt =
  //      featureTexturePropertyMap.Find(pImage);
  //  if (pMappedUnrealImageIt) {
  //    encodedFeatureTextureProperty.pTexture = pMappedUnrealImageIt->Pin();
  //  } else {
  //    encodedFeatureTextureProperty.pTexture =
  //        MakeShared<LoadedTextureResult>();
  //    // TODO: upgrade to new texture creation path.
  //    encodedFeatureTextureProperty.pTexture->textureSource =
  //        LegacyTextureSource{};
  //    featureTexturePropertyMap.Emplace(
  //        pImage,
  //        encodedFeatureTextureProperty.pTexture);
  //    encodedFeatureTextureProperty.pTexture->pTextureData =
  //        createTexturePlatformData(
  //            pImage->width,
  //            pImage->height,
  //            // TODO: currently the unnormalized pixels are always in
  //            // unsigned R8G8B8A8 form, but this does not necessarily need
  //            // to be the case in the future.
  //            featureTexturePropertyView.isNormalized()
  //                ? EPixelFormat::PF_R8G8B8A8
  //                : EPixelFormat::PF_R8G8B8A8_UINT);

  //    encodedFeatureTextureProperty.pTexture->addressX =
  //        TextureAddress::TA_Clamp;
  //    encodedFeatureTextureProperty.pTexture->addressY =
  //        TextureAddress::TA_Clamp;
  //    encodedFeatureTextureProperty.pTexture->filter =
  //        TextureFilter::TF_Nearest;

  //    if (!encodedFeatureTextureProperty.pTexture->pTextureData) {
  //      UE_LOG(
  //          LogCesium,
  //          Error,
  //          TEXT(
  //              "Error encoding a feature table property. Most likely could
  //              not allocate enough texture memory."));
  //      continue;
  //    }

  //    FTexture2DMipMap* pMip = new FTexture2DMipMap();
  //    encodedFeatureTextureProperty.pTexture->pTextureData->Mips.Add(pMip);
  //    pMip->SizeX = pImage->width;
  //    pMip->SizeY = pImage->height;
  //    pMip->BulkData.Lock(LOCK_READ_WRITE);

  //    void* pTextureData = pMip->BulkData.Realloc(pImage->pixelData.size());

  //    FMemory::Memcpy(
  //        pTextureData,
  //        pImage->pixelData.data(),
  //        pImage->pixelData.size());

  //    pMip->BulkData.Unlock();
  //  }
  //}

  return encodedPropertyTexture;
}

EncodedPrimitiveMetadata encodePrimitiveMetadataAnyThreadPart(
    const FMetadataDescription& metadataDescription,
    const FCesiumPrimitiveMetadata& primitive) {

  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::EncodeMetadataPrimitive)

  EncodedPrimitiveMetadata result;

  //const TArray<FCesiumFeatureIdTexture>& featureIdTextures =
  //    UCesiumMetadataPrimitiveBlueprintLibrary::GetFeatureIdTextures(primitive);
  //const TArray<FCesiumFeatureIdAttribute>& featureIdAttributes =
  //    UCesiumMetadataPrimitiveBlueprintLibrary::GetFeatureIdAttributes(
  //        primitive);

  //const TArray<FString>& featureTextureNames =
  //    UCesiumMetadataPrimitiveBlueprintLibrary::GetFeatureTextureNames(
  //        primitive);
  //result.featureTextureNames.Reserve(featureTextureNames.Num());

  //for (const FFeatureTextureDescription& expectedFeatureTexture :
  //     metadataDescription.FeatureTextures) {
  //  if (featureTextureNames.Find(expectedFeatureTexture.Name) != INDEX_NONE) {
  //    result.featureTextureNames.Add(expectedFeatureTexture.Name);
  //  }
  //}

  //TMap<const CesiumGltf::ImageCesium*, TWeakPtr<LoadedTextureResult>>
  //    featureIdTextureMap;
  //featureIdTextureMap.Reserve(featureIdTextures.Num());

  //result.encodedFeatureIdTextures.Reserve(featureIdTextures.Num());
  //result.encodedFeatureIdAttributes.Reserve(featureIdAttributes.Num());

  //// Imposed implementation limitation: Assume only upto one feature id texture
  //// or attribute corresponds to each feature table.
  //for (const FFeatureTableDescription& expectedFeatureTable :
  //     metadataDescription.FeatureTables) {
  //  const FString& featureTableName = expectedFeatureTable.Name;

  //  if (expectedFeatureTable.AccessType ==
  //      ECesiumFeatureTableAccessType::Texture) {

  //    const FCesiumFeatureIdTexture* pFeatureIdTexture =
  //        featureIdTextures.FindByPredicate([&featureTableName](
  //                                              const FCesiumFeatureIdTexture&
  //                                                  featureIdTexture) {
  //          return featureTableName ==
  //                 UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureTableName(
  //                     featureIdTexture);
  //        });

  //    if (pFeatureIdTexture) {
  //      const CesiumGltf::FeatureIDTextureView& featureIdTextureView =
  //          pFeatureIdTexture->getFeatureIdTextureView();
  //      const CesiumGltf::ImageCesium* pFeatureIdImage =
  //          featureIdTextureView.getImage();

  //      if (!pFeatureIdImage) {
  //        UE_LOG(
  //            LogCesium,
  //            Warning,
  //            TEXT("Feature id texture missing valid image."));
  //        continue;
  //      }

  //      TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::EncodeFeatureIdTexture)

  //      EncodedFeatureIdTexture& encodedFeatureIdTexture =
  //          result.encodedFeatureIdTextures.Emplace_GetRef();

  //      encodedFeatureIdTexture.baseName = "FIT_" + featureTableName + "_";
  //      encodedFeatureIdTexture.channel = featureIdTextureView.getChannel();
  //      encodedFeatureIdTexture.textureCoordinateAttributeId =
  //          featureIdTextureView.getTextureCoordinateAttributeId();

  //      TWeakPtr<LoadedTextureResult>* pMappedUnrealImageIt =
  //          featureIdTextureMap.Find(pFeatureIdImage);
  //      if (pMappedUnrealImageIt) {
  //        encodedFeatureIdTexture.pTexture = pMappedUnrealImageIt->Pin();
  //      } else {
  //        encodedFeatureIdTexture.pTexture = MakeShared<LoadedTextureResult>();
  //        // TODO: upgrade to new texture creation path
  //        encodedFeatureIdTexture.pTexture->textureSource =
  //            LegacyTextureSource{};
  //        featureIdTextureMap.Emplace(
  //            pFeatureIdImage,
  //            encodedFeatureIdTexture.pTexture);
  //        encodedFeatureIdTexture.pTexture->pTextureData =
  //            createTexturePlatformData(
  //                pFeatureIdImage->width,
  //                pFeatureIdImage->height,
  //                // TODO: currently this is always the case, but doesn't have
  //                // to be
  //                EPixelFormat::PF_R8G8B8A8_UINT);

  //        encodedFeatureIdTexture.pTexture->addressX = TextureAddress::TA_Clamp;
  //        encodedFeatureIdTexture.pTexture->addressY = TextureAddress::TA_Clamp;
  //        encodedFeatureIdTexture.pTexture->filter = TextureFilter::TF_Nearest;

  //        if (!encodedFeatureIdTexture.pTexture->pTextureData) {
  //          UE_LOG(
  //              LogCesium,
  //              Error,
  //              TEXT(
  //                  "Error encoding a feature table property. Most likely could not allocate enough texture memory."));
  //          continue;
  //        }

  //        FTexture2DMipMap* pMip = new FTexture2DMipMap();
  //        encodedFeatureIdTexture.pTexture->pTextureData->Mips.Add(pMip);
  //        pMip->SizeX = pFeatureIdImage->width;
  //        pMip->SizeY = pFeatureIdImage->height;
  //        pMip->BulkData.Lock(LOCK_READ_WRITE);

  //        void* pTextureData =
  //            pMip->BulkData.Realloc(pFeatureIdImage->pixelData.size());

  //        FMemory::Memcpy(
  //            pTextureData,
  //            pFeatureIdImage->pixelData.data(),
  //            pFeatureIdImage->pixelData.size());

  //        pMip->BulkData.Unlock();
  //      }

  //      encodedFeatureIdTexture.featureTableName = featureTableName;
  //    }
  //  } else if (
  //      expectedFeatureTable.AccessType ==
  //      ECesiumFeatureTableAccessType::Attribute) {
  //    for (size_t i = 0; i < featureIdAttributes.Num(); ++i) {
  //      const FCesiumFeatureIdAttribute& featureIdAttribute =
  //          featureIdAttributes[i];

  //      if (featureTableName ==
  //          UCesiumFeatureIdAttributeBlueprintLibrary::GetFeatureTableName(
  //              featureIdAttribute)) {
  //        EncodedFeatureIdAttribute& encodedFeatureIdAttribute =
  //            result.encodedFeatureIdAttributes.Emplace_GetRef();

  //        encodedFeatureIdAttribute.name = "FA_" + featureTableName;
  //        encodedFeatureIdAttribute.featureTableName = featureTableName;
  //        encodedFeatureIdAttribute.index = static_cast<int32>(i);

  //        break;
  //      }
  //    }
  //  }
  //}

  return result;
}

EncodedModelMetadata encodeModelMetadataAnyThreadPart(
    const FMetadataDescription& metadataDescription,
    const FCesiumModelMetadata& metadata) {

  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::EncodeMetadataModel)

  EncodedModelMetadata result;

  const TArray<FCesiumPropertyTable>& propertyTables =
      UCesiumModelMetadataBlueprintLibrary::GetPropertyTables(metadata);
  result.propertyTables.Reserve(propertyTables.Num());
  for (const auto& propertyTableIt : propertyTables) {
    // const FString& featureTableName = propertyTableIt.Key;

    // const FFeatureTableDescription* pExpectedFeatureTable =
    //    metadataDescription.FeatureTables.FindByPredicate(
    //        [&featureTableName](
    //            const FFeatureTableDescription& expectedFeatureTable) {
    //          return featureTableName == expectedFeatureTable.Name;
    //        });

    // if (pExpectedFeatureTable) {
    //  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::EncodeFeatureTable)

    //  result.encodedFeatureTables.Emplace(
    //      featureTableName,
    //      encodeMetadataFeatureTableAnyThreadPart(
    //          *pExpectedFeatureTable,
    //          featureTableIt.Value));
    //}
  }

  const TArray<FCesiumPropertyTexture>& propertyTextures =
      UCesiumModelMetadataBlueprintLibrary::GetPropertyTextures(metadata);
  result.propertyTextures.Reserve(propertyTextures.Num());
  TMap<const CesiumGltf::ImageCesium*, TWeakPtr<LoadedTextureResult>>
      propertyTexturePropertyMap;
  propertyTexturePropertyMap.Reserve(propertyTextures.Num());
  for (const auto& propertyTextureIt : propertyTextures) {
    // const FString& featureTextureName = featureTextureIt.Key;

    // const FFeatureTextureDescription* pExpectedFeatureTexture =
    //    metadataDescription.FeatureTextures.FindByPredicate(
    //        [&featureTextureName](
    //            const FFeatureTextureDescription& expectedFeatureTexture) {
    //          return featureTextureName == expectedFeatureTexture.Name;
    //        });

    // if (pExpectedFeatureTexture) {
    //  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::EncodeFeatureTexture)

    //  result.encodedFeatureTextures.Emplace(
    //      featureTextureName,
    //      encodeFeatureTextureAnyThreadPart(
    //          featureTexturePropertyMap,
    //          *pExpectedFeatureTexture,
    //          featureTextureName,
    //          featureTextureIt.Value));
    //}
  }

  return result;
}

bool encodePropertyTableGameThreadPart(
    EncodedPropertyTable& encodedPropertyTable) {
  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::EncodePropertyTable)

  bool success = true;

  for (EncodedPropertyTableProperty& encodedProperty :
       encodedPropertyTable.properties) {
    success &=
        loadTextureGameThreadPart(encodedProperty.pTexture.Get()) != nullptr;
  }

  return success;
}

bool encodePropertyTextureGameThreadPart(
    TArray<LoadedTextureResult*>& uniqueTextures,
    EncodedPropertyTexture& encodedPropertyTexture) {
  bool success = true;

  for (EncodedPropertyTextureProperty& property :
       encodedPropertyTexture.properties) {
    if (uniqueTextures.Find(property.pTexture.Get()) == INDEX_NONE) {
      success &= loadTextureGameThreadPart(property.pTexture.Get()) != nullptr;
      uniqueTextures.Emplace(property.pTexture.Get());
    }
  }

  return success;
}

bool encodePrimitiveMetadataGameThreadPart(
    EncodedPrimitiveMetadata& encodedPrimitive) {
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

bool encodeModelMetadataGameThreadPart(EncodedModelMetadata& encodedMetadata) {
  TRACE_CPUPROFILER_EVENT_SCOPE(Cesium::EncodeMetadata)

  bool success = true;

  TArray<LoadedTextureResult*> uniqueTextures;
  uniqueTextures.Reserve(encodedMetadata.propertyTextures.Num());
  for (auto& encodedPropertyTextureIt : encodedMetadata.propertyTextures) {
    success &= encodePropertyTextureGameThreadPart(
        uniqueTextures,
        encodedPropertyTextureIt.Value);
  }

  for (auto& encodedPropertyTableIt : encodedMetadata.propertyTables) {
    success &=
        encodePropertyTableGameThreadPart(encodedPropertyTableIt.Value);
  }

  return success;
}

void destroyEncodedPrimitiveMetadata(
    EncodedPrimitiveMetadata& encodedPrimitive) {
  for (EncodedFeatureIdTexture& encodedFeatureIdTexture :
       encodedPrimitive.encodedFeatureIdTextures) {

    if (encodedFeatureIdTexture.pTexture->pTexture.IsValid()) {
      CesiumLifetime::destroy(encodedFeatureIdTexture.pTexture->pTexture.Get());
      encodedFeatureIdTexture.pTexture->pTexture.Reset();
    }
  }
}

void destroyEncodedModelMetadata(EncodedModelMetadata& encodedMetadata) {
  for (auto& propertyTableIt : encodedMetadata.propertyTables) {
    for (EncodedPropertyTableProperty& encodedProperty :
         propertyTableIt.Value.properties) {
      if (encodedProperty.pTexture->pTexture.IsValid()) {
        CesiumLifetime::destroy(encodedProperty.pTexture->pTexture.Get());
        encodedProperty.pTexture->pTexture.Reset();
      }
    }
  }

  for (auto& encodedFeatureTextureIt : encodedMetadata.propertyTextures) {
    for (EncodedPropertyTextureProperty& encodedFeatureTextureProperty :
         encodedFeatureTextureIt.Value.properties) {
      if (encodedFeatureTextureProperty.pTexture->pTexture.IsValid()) {
        CesiumLifetime::destroy(
            encodedFeatureTextureProperty.pTexture->pTexture.Get());
        encodedFeatureTextureProperty.pTexture->pTexture.Reset();
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

// clang-format on
