// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/Model.h"
#include "CesiumMetadataValueType.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include <optional>
#include <vector>

#include "Containers/UnrealString.h"

struct FCesiumMetadataPrimitive;
struct FCesiumMetadataFeatureTable;

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
    std::vector<EncodedMetadataProperty> encodedProperties;
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
    EncodedMetadataFeatureTable encodedFeatureTable;

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
    EncodedMetadataFeatureTable encodedFeatureTable;
  };

  struct EncodedMetadataPrimitive {
    std::vector<EncodedFeatureIdTexture> encodedFeatureIdTextures;
    std::vector<EncodedVertexMetadata> encodedVertexMetadata;
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
      const FCesiumMetadataFeatureTable& featureTable);

  static EncodedMetadataPrimitive encodeMetadataPrimitiveAnyThreadPart(
      const FCesiumMetadataPrimitive& primitive);

  static bool encodeMetadataFeatureTableGameThreadPart(
      EncodedMetadataFeatureTable& encodedFeatureTable);

  static bool encodeMetadataPrimitiveGameThreadPart(
      EncodedMetadataPrimitive& encodedPrimitive);
};
