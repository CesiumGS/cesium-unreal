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

class CesiumTextureUtility {
public:
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

  // TODO: some changes are needed here to avoid duplicated encoded feature
  // tables across multiple feature id textures/attributes
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
  };

  struct EncodedFeatureTextureProperty {
    FString name;
    LoadedTextureResult* pTexture;
    int64 textureCoordinateIndex;
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
  static LoadedTextureResult* loadTextureAnyThreadPart(
      const CesiumGltf::ImageCesium& image,
      const TextureAddress& addressX,
      const TextureAddress& addressY,
      const TextureFilter& filter);

  static LoadedTextureResult* loadTextureAnyThreadPart(
      const CesiumGltf::Model& model,
      const CesiumGltf::Texture& texture);

  static bool
  loadTextureGameThreadPart(LoadedTextureResult* pHalfLoadedTexture);

  static EncodedMetadataFeatureTable encodeMetadataFeatureTableAnyThreadPart(
      const FString& featureTableName,
      const FCesiumMetadataFeatureTable& featureTable);

  static EncodedFeatureTexture encodeFeatureTextureAnyThreadPart(
      TMap<const CesiumGltf::ImageCesium*, LoadedTextureResult*>&
          featureTexturePropertyMap,
      const FString& featureTextureName,
      const FCesiumFeatureTexture& featureTexture);

  static EncodedMetadataPrimitive encodeMetadataPrimitiveAnyThreadPart(
      const FCesiumMetadataPrimitive& primitive);

  static EncodedMetadata
  encodeMetadataAnyThreadPart(const FCesiumMetadataModel& metadata);

  static bool encodeMetadataFeatureTableGameThreadPart(
      EncodedMetadataFeatureTable& encodedFeatureTable);

  static bool encodeFeatureTextureGameThreadPart(
      TArray<LoadedTextureResult*>& uniqueTextures,
      EncodedFeatureTexture& encodedFeatureTexture);

  static bool encodeMetadataPrimitiveGameThreadPart(
      EncodedMetadataPrimitive& encodedPrimitive);

  static bool encodeMetadataGameThreadPart(EncodedMetadata& encodedMetadata);

  static void
  destroyEncodedMetadataPrimitive(EncodedMetadataPrimitive& encodedPrimitive);

  static void destroyEncodedMetadata(EncodedMetadata& encodedMetadata);
};
