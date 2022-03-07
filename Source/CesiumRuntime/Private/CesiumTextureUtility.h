// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/Model.h"
#include "CesiumMetadataValueType.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include <optional>

#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Containers/UnrealString.h"

namespace CesiumGltf {
struct ImageCesium;
} // namespace CesiumGltf

struct FCesiumMetadataModel;
struct FCesiumMetadataPrimitive;
struct FCesiumMetadataFeatureTable;
struct FCesiumFeatureTexture;
struct FFeatureTableDescription;

class UCesiumEncodedMetadataComponent;

namespace CesiumTextureUtility {
struct LoadedTextureResult {
  FTexturePlatformData* pTextureData;
  TextureAddress addressX;
  TextureAddress addressY;
  TextureFilter filter;
  UTexture2D* pTexture;
};

struct EncodedMetadataProperty {
  /**
   * @brief The name of this property.
   */
  FString name;

  /**
   * @brief The encoded property array.
   */
  LoadedTextureResult* pTexture;
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
  FString name;

  /**
   * @brief The encoded feature table corresponding to this feature id
   * texture.
   */
  FString featureTableName;

  /**
   * @brief The actual feature id texture.
   */
  LoadedTextureResult* pTexture;

  /**
   * @brief The channel that this feature id texture uses within the image.
   */
  int32 channel;

  /**
   * @brief The texture coordinate index for the feature id texture.
   */
  int64 textureCoordinateIndex;
};

struct EncodedVertexMetadata {
  FString name;
  FString featureTableName;
  int32 textureCoordinateIndex;
  int32 index;
};

struct EncodedFeatureTextureProperty {
  FString name;
  LoadedTextureResult* pTexture;
  int64 textureCoordinateIndex;
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

// TODO: documentation
LoadedTextureResult* loadTextureAnyThreadPart(
    const CesiumGltf::ImageCesium& image,
    const TextureAddress& addressX,
    const TextureAddress& addressY,
    const TextureFilter& filter);

LoadedTextureResult* loadTextureAnyThreadPart(
    const CesiumGltf::Model& model,
    const CesiumGltf::Texture& texture);

bool loadTextureGameThreadPart(LoadedTextureResult* pHalfLoadedTexture);

EncodedMetadataFeatureTable encodeMetadataFeatureTableAnyThreadPart(
    const FFeatureTableDescription& encodeInstructions,
    const FCesiumMetadataFeatureTable& featureTable);

EncodedFeatureTexture encodeFeatureTextureAnyThreadPart(
    TMap<const CesiumGltf::ImageCesium*, LoadedTextureResult*>&
        featureTexturePropertyMap,
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
    TArray<LoadedTextureResult*>& uniqueTextures,
    EncodedFeatureTexture& encodedFeatureTexture);

bool encodeMetadataPrimitiveGameThreadPart(
    EncodedMetadataPrimitive& encodedPrimitive);

bool encodeMetadataGameThreadPart(EncodedMetadata& encodedMetadata);

void destroyEncodedMetadataPrimitive(
    EncodedMetadataPrimitive& encodedPrimitive);

void destroyEncodedMetadata(EncodedMetadata& encodedMetadata);
} // namespace CesiumTextureUtility
