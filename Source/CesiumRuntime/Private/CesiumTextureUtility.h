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
     * @brief The type of this property.
     */
    ECesiumMetadataBlueprintType type;

    /**
     * @brief The type of the components of this property.
     *
     * Only applicable if type is Array.
     */
    ECesiumMetadataBlueprintType componentType;

    /**
     * @brief The encoded property array.
     */
    LoadedTextureResult* pTexture;
  };

  struct EncodedMetadataFeatureTable {
    /**
     * @brief The encoded properties in this feature table.
     */
    std::vector<EncodedMetadataProperty> encodedProperties;
  };

  struct EncodedFeatureIdTexture {
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
  };

  struct EncodedVertexMetadata {
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

  static EncodedMetadataFeatureTable
  encodeMetadataFeatureTable(const FCesiumMetadataFeatureTable& featureTable);

  static EncodedMetadataPrimitive
  encodeMetadataPrimitive(const FCesiumMetadataPrimitive& primitive);
};
