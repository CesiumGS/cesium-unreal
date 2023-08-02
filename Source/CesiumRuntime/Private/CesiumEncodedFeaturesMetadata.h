// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumMetadataValueType.h"
#include "CesiumTextureUtility.h"
#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Containers/UnrealString.h"
#include "Templates/SharedPointer.h"
#include "Templates/UniquePtr.h"
#include <variant>

struct FCesiumFeatureIdSet;
struct FCesiumPrimitiveFeatures;
struct FCesiumModelMetadata;
struct FCesiumPrimitiveMetadata;
struct FCesiumPropertyTable;
struct FCesiumPropertyTexture;
struct FCesiumPropertyTableDescription;
struct FFeatureTextureDescription;
struct FCesiumModelMetadataDescription;
struct FCesiumPrimitiveFeaturesDescription;

/**
 * @brief Provides utility for encoding feature IDs from EXT_mesh_features and
 * metadata from EXT_structural_metadata. "Encoding" refers broadly to the
 * process of converting data to accessible formats on the GPU, and giving them
 * names for use in Unreal materials.
 *
 * First, the desired feature ID sets / metadata properties from across the
 * tileset must be generated on a CesiumFeaturesMetadataComponent. Then,
 * encoding occurs on a model-by-model basis. Not all models in a tileset may
 * necessarily contain the feature IDs / metadata specified in the description.
 */
namespace CesiumEncodedFeaturesMetadata {

/**
 * Naming convention for encoded features + metadata material parameters
 *
 * Feature Id Textures:
 *  - Base: "FIT_<feature table name>_"...
 *    - Texture: ..."TX"
 *    - Texture Coordinate Index: ..."UV"
 *    - Channel Mask: ..."CM"
 *
 * Feature Texture Properties:
 *  - Base: "FTX_<feature texture name>_<property name>_"...
 *    - Texture: ..."TX"
 *    - Texture Coordinate Index: ..."UV"
 *    - Swizzle: ..."SW"
 *
 * Encoded Feature Table Properties:
 *  - Encoded Property Table:
 *    "FTB_<feature table name>_<property name>"
 */
static const FString MaterialTextureSuffix = "_TX";
static const FString MaterialTexCoordSuffix = "_UV";
static const FString MaterialChannelsSuffix = "_CHANNELS";
static const FString MaterialNumChannelsSuffix = "_NUM_CHANNELS";

#pragma region Encoded Primitive Features

/**
 * @brief Generates a name for a feature ID set in a glTF primitive's
 * EXT_mesh_features. If the feature ID set already has a label, this will
 * return the label. Otherwise, if the feature ID set is unlabeled, a name will
 * be generated like so:
 *
 * - If the feature ID set is an attribute, this will appear as
 * "_FEATURE_ID_<index>", where <index> is the set index specified in
 * the attribute.
 * - If the feature ID set is a texture, this will appear as
 * "_FEATURE_ID_TEXTURE_<index>", where <index> increments with the number of
 * feature ID textures seen in an individual primitive.
 * - If the feature ID set is an implicit set, this will appear as
 * "_IMPLICIT_FEATURE_ID". Implicit feature ID sets don't vary in definition,
 * so any additional implicit feature ID sets across the primitives are
 * counted by this one.
 *
 * This is also used by FCesiumFeatureIdSetDescription to display the names of
 * the feature ID sets across a tileset.
 *
 * @param FeatureIDSet The feature ID set
 * @param FeatureIDTextureCounter The counter representing how many feature ID
 * textures have been seen in the primitive thus far. Will be incremented by
 * this function if the given feature ID set is a texture.
 */
FString getNameForFeatureIDSet(
    const FCesiumFeatureIdSet& FeatureIDSet,
    int32& FeatureIDTextureCounter);

/**
 * @brief A feature ID texture that has been encoded for access on the GPU.
 */
struct EncodedFeatureIdTexture {
  /**
   * @brief The actual feature ID texture.
   */
  TSharedPtr<CesiumTextureUtility::LoadedTextureResult> pTexture;

  /**
   * @brief The channels that this feature ID texture uses within the image.
   */
  std::vector<int64_t> channels;

  /**
   * @brief The set index of the texture coordinates used to sample this feature
   * ID texture.
   */
  int64 textureCoordinateSetIndex;
};

/**
 * @brief A feature ID set that has been encoded for access on the GPU.
 */
struct EncodedFeatureIdSet {
  /**
   * @brief The name assigned to this feature ID set. This will be used as a
   * variable name in the generated Unreal material.
   */
  FString name;

  /**
   * @brief The index of this feature ID set in the FCesiumPrimitiveFeatures on
   * the glTF primitive.
   */
  int32 index;

  /**
   * @brief The set index of the feature ID attribute. This is an integer value
   * used to construct a string in the format "_FEATURE_ID_<set index>",
   * corresponding to a glTF primitive attribute of the same name. Only
   * applicable if the feature ID set represents a feature ID attribute.
   */
  std::optional<int32> attribute;

  /**
   * @brief The encoded feature ID texture. Only applicable if the feature ID
   * set represents a feature ID texture.
   */
  std::optional<EncodedFeatureIdTexture> texture;

  /**
   * @brief The name of the property table that this feature ID set corresponds
   * to. Only applicable if the model contains the `EXT_structural_metadata`
   * extension.
   */
  FString propertyTableName;
};

/**
 * @brief The encoded representation of the EXT_mesh_features of a glTF
 * primitive.
 */
struct EncodedPrimitiveFeatures {
  TArray<EncodedFeatureIdSet> featureIdSets;
};

/**
 * @brief Prepares the EXT_mesh_features of a glTF primitive to be encoded, for
 * use with Unreal Engine materials. This only encodes the feature ID sets
 * specified by the FCesiumPrimitiveFeaturesDescription.
 */
EncodedPrimitiveFeatures encodePrimitiveFeaturesAnyThreadPart(
    const FCesiumPrimitiveFeaturesDescription& featuresDescription,
    const FCesiumPrimitiveFeatures& features);

/**
 * @brief Encodes the EXT_mesh_features of a glTF primitive for use with Unreal
 * Engine materials.
 *
 * @returns True if the encoding of all feature ID sets was successful, false
 * otherwise.
 */
bool encodePrimitiveFeaturesGameThreadPart(
    EncodedPrimitiveFeatures& encodedFeatures);

void destroyEncodedPrimitiveFeatures(EncodedPrimitiveFeatures& encodedFeatures);

#pragma endregion

/**
 * A property table property that has been encoded for access on the GPU.
 */
struct EncodedPropertyTableProperty {
  /**
   * @brief The name of this property table property.
   */
  FString name;

  /**
   * @brief The property table property values, encoded into a texture.
   */
  TUniquePtr<CesiumTextureUtility::LoadedTextureResult> pTexture;
};

/**
 * A property table whose properties have been encoded for access on the GPU.
 */
struct EncodedPropertyTable {
  /**
   * @brief The encoded properties in this property table.
   */
  TArray<EncodedPropertyTableProperty> properties;
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
  TArray<FString> propertyTextureNames;
};

struct EncodedModelMetadata {
  TMap<FString, EncodedPropertyTable> propertyTables;
  TMap<FString, EncodedPropertyTexture> propertyTextures;
};

EncodedPropertyTable encodePropertyTableAnyThreadPart(
    const FCesiumPropertyTableDescription& featureTableDescription,
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
    const FCesiumModelMetadataDescription& metadataDescription,
    const FCesiumPrimitiveFeatures& features,
    const FCesiumPrimitiveMetadata& primitive);

EncodedModelMetadata encodeModelMetadataAnyThreadPart(
    const FCesiumModelMetadataDescription& metadataDescription,
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

} // namespace CesiumEncodedFeaturesMetadata
