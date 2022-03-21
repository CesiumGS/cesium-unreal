
#pragma once

#include "CesiumMetadataValueType.h"
#include "CesiumTextureUtility.h"
#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Containers/UnrealString.h"
#include "Templates/SharedPointer.h"
#include "Templates/UniquePtr.h"

struct FCesiumMetadataModel;
struct FCesiumMetadataPrimitive;
struct FCesiumMetadataFeatureTable;
struct FCesiumFeatureTexture;
struct FFeatureTableDescription;
struct FFeatureTextureDescription;

class UCesiumEncodedMetadataComponent;

namespace CesiumEncodedMetadataUtility {
struct EncodedMetadataProperty {
  /**
   * @brief The name of this property.
   */
  FString name;

  /**
   * @brief The encoded property array.
   */
  TUniquePtr<CesiumTextureUtility::LoadedTextureResult> pTexture;
};

struct EncodedMetadataFeatureTable {
  /**
   * @brief The encoded properties in this feature table.
   */
  TArray<EncodedMetadataProperty> encodedProperties;
};

struct EncodedFeatureIdTexture {
  /**
   * @brief The name to use for this feature id texture in the shader.
   */
  FString baseName;

  /**
   * @brief The encoded feature table corresponding to this feature id
   * texture.
   */
  FString featureTableName;

  /**
   * @brief The actual feature id texture.
   */
  TSharedPtr<CesiumTextureUtility::LoadedTextureResult> pTexture;

  /**
   * @brief The channel that this feature id texture uses within the image.
   */
  int32 channel;

  /**
   * @brief The texture coordinate accessor index for the feature id texture.
   */
  int64 textureCoordinateAttributeId;
};

struct EncodedVertexMetadata {
  FString name;
  FString featureTableName;
  int32 index;
};

struct EncodedFeatureTextureProperty {
  FString baseName;
  TSharedPtr<CesiumTextureUtility::LoadedTextureResult> pTexture;
  int64 textureCoordinateAttributeId;
  int32 channelOffsets[4];
};

struct EncodedFeatureTexture {
  TArray<EncodedFeatureTextureProperty> properties;
};

struct EncodedMetadataPrimitive {
  TArray<EncodedFeatureIdTexture> encodedFeatureIdTextures;
  TArray<EncodedVertexMetadata> encodedVertexMetadata;
  TArray<FString> featureTextureNames;
};

struct EncodedMetadata {
  TMap<FString, EncodedMetadataFeatureTable> encodedFeatureTables;
  TMap<FString, EncodedFeatureTexture> encodedFeatureTextures;
};

EncodedMetadataFeatureTable encodeMetadataFeatureTableAnyThreadPart(
    const FFeatureTableDescription& encodeInstructions,
    const FCesiumMetadataFeatureTable& featureTable);

EncodedFeatureTexture encodeFeatureTextureAnyThreadPart(
    TMap<
        const CesiumGltf::ImageCesium*,
        TWeakPtr<CesiumTextureUtility::LoadedTextureResult>>&
        featureTexturePropertyMap,
    const FFeatureTextureDescription& featureTextureDescription,
    const FString& featureTextureName,
    const FCesiumFeatureTexture& featureTexture);

EncodedMetadataPrimitive encodeMetadataPrimitiveAnyThreadPart(
    const UCesiumEncodedMetadataComponent& encodedInformation,
    const FCesiumMetadataPrimitive& primitive);

EncodedMetadata encodeMetadataAnyThreadPart(
    const UCesiumEncodedMetadataComponent& encodeInstructions,
    const FCesiumMetadataModel& metadata);

bool encodeMetadataFeatureTableGameThreadPart(
    EncodedMetadataFeatureTable& encodedFeatureTable);

bool encodeFeatureTextureGameThreadPart(
    TArray<TUniquePtr<CesiumTextureUtility::LoadedTextureResult>>&
        uniqueTextures,
    EncodedFeatureTexture& encodedFeatureTexture);

bool encodeMetadataPrimitiveGameThreadPart(
    EncodedMetadataPrimitive& encodedPrimitive);

bool encodeMetadataGameThreadPart(EncodedMetadata& encodedMetadata);

void destroyEncodedMetadataPrimitive(
    EncodedMetadataPrimitive& encodedPrimitive);

void destroyEncodedMetadata(EncodedMetadata& encodedMetadata);
} // namespace CesiumEncodedMetadataUtility
