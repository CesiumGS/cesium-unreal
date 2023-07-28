// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumMetadataValueType.h"
#include "CesiumTextureUtility.h"
#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Containers/UnrealString.h"
#include "Templates/SharedPointer.h"
#include "Templates/UniquePtr.h"

struct FCesiumModelMetadata;
struct FCesiumPrimitiveMetadata;
struct FCesiumPropertyTable;
struct FCesiumPropertyTexture;
struct FFeatureTableDescription;
struct FFeatureTextureDescription;
struct FMetadataDescription;

namespace CesiumEncodedMetadataUtility {
struct EncodedPropertyTableProperty {
  /**
   * @brief The name of this property tableproperty.
   */
  FString name;

  /**
   * @brief The property table property values, encoded into a texture.
   */
  TUniquePtr<CesiumTextureUtility::LoadedTextureResult> pTexture;
};

struct EncodedPropertyTable {
  /**
   * @brief The encoded properties in this property table.
   */
  TArray<EncodedPropertyTableProperty> properties;
};

struct EncodedFeatureIdTexture {
  /**
   * @brief The name to use for this feature ID texture in the shader.
   */
  FString baseName;

  /**
   * @brief The encoded property table corresponding to this feature ID
   * texture.
   */
  FString propertyTableName;

  /**
   * @brief The actual feature ID texture.
   */
  TSharedPtr<CesiumTextureUtility::LoadedTextureResult> pTexture;

  /**
   * @brief The channel that this feature ID texture uses within the image.
   */
  int32 channel;

  /**
   * @brief The texture coordinate accessor index for the feature ID texture.
   */
  int64 textureCoordinateAttributeId;
};

struct EncodedFeatureIdAttribute {
  FString name;
  FString featureTableName;
  int32 index;
};

struct EncodedPropertyTextureProperty {
  FString baseName;
  TSharedPtr<CesiumTextureUtility::LoadedTextureResult> pTexture;
  int64 textureCoordinateAttributeId;
  int32 channelOffsets[4];
};

struct EncodedPropertyTexture {
  TArray<EncodedPropertyTextureProperty> properties;
};

struct EncodedPrimitiveMetadata {
  TArray<EncodedFeatureIdTexture> encodedFeatureIdTextures;
  TArray<EncodedFeatureIdAttribute> encodedFeatureIdAttributes;
  TArray<FString> propertyTextureNames;
};

struct EncodedModelMetadata {
  TMap<FString, EncodedPropertyTable> propertyTables;
  TMap<FString, EncodedPropertyTexture> propertyTextures;
};

EncodedPropertyTable encodePropertyTableAnyThreadPart(
    const FFeatureTableDescription& featureTableDescription,
    const FCesiumPropertyTable& propertyTable);

EncodedPropertyTexture encodePropertyTextureAnyThreadPart(
    TMap<
        const CesiumGltf::ImageCesium*,
        TWeakPtr<CesiumTextureUtility::LoadedTextureResult>>&
        propertyTexturePropertyMap,
    const FFeatureTextureDescription& featureTextureDescription,
    const FString& propertyTextureName,
    const FCesiumPropertyTexture& propertyTexture);

EncodedPrimitiveMetadata encodePrimitiveMetadataAnyThreadPart(
    const FMetadataDescription& metadataDescription,
    const FCesiumPrimitiveMetadata& primitive);

EncodedModelMetadata encodeModelMetadataAnyThreadPart(
    const FMetadataDescription& metadataDescription,
    const FCesiumModelMetadata& modelMetadata);

bool encodePropertyTableGameThreadPart(
    EncodedPropertyTable& encodedFeatureTable);

bool encodePropertyTextureGameThreadPart(
    TArray<TUniquePtr<CesiumTextureUtility::LoadedTextureResult>>&
        uniqueTextures,
    EncodedPropertyTexture& encodedFeatureTexture);

bool encodePrimitiveMetadataGameThreadPart(
    EncodedPrimitiveMetadata& encodedPrimitive);

bool encodeModelMetadataGameThreadPart(EncodedModelMetadata& encodedMetadata);

void destroyEncodedPrimitiveMetadata(
    EncodedPrimitiveMetadata& encodedPrimitive);

void destroyEncodedModelMetadata(EncodedModelMetadata& encodedMetadata);

FString createHlslSafeName(const FString& rawName);

} // namespace CesiumEncodedMetadataUtility
