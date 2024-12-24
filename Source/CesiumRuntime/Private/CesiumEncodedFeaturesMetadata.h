// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumMetadataEncodingDetails.h"
#include "CesiumMetadataValue.h"
#include "CesiumTextureUtility.h"
#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Containers/UnrealString.h"
#include "Templates/SharedPointer.h"
#include "Templates/UniquePtr.h"
#include <CesiumGltf/KhrTextureTransform.h>
#include <array>
#include <variant>

struct FCesiumFeatureIdSet;
struct FCesiumPrimitiveFeatures;
struct FCesiumModelMetadata;
struct FCesiumPrimitiveMetadata;
struct FCesiumPropertyTable;
struct FCesiumPropertyTableProperty;
struct FCesiumPropertyTexture;
struct FCesiumPropertyTableDescription;
struct FCesiumPropertyTextureDescription;
struct FFeatureTextureDescription;
struct FCesiumModelMetadataDescription;
struct FCesiumPrimitiveFeaturesDescription;
struct FCesiumPrimitiveMetadataDescription;

struct FCesiumMetadataPropertyDetails;
class UMaterialInstanceDynamic;
enum EMaterialParameterAssociation : int;

/**
 * @brief Provides utility for encoding feature IDs from EXT_mesh_features and
 * metadata from EXT_structural_metadata. "Encoding" refers broadly to the
 * process of converting data to accessible formats on the GPU. This process
 * also gives them names for use in Unreal materials.
 *
 * First, the desired feature ID sets / metadata properties must be filled out
 * on a tileset's CesiumFeaturesMetadataComponent. Then, encoding will occur on
 * a model-by-model basis. Not all models in a tileset may necessarily contain
 * the feature IDs / metadata specified in the description.
 */
namespace CesiumEncodedFeaturesMetadata {

/**
 * Naming convention for feature ID texture parameters nodes:
 *  - Texture: FeatureIDTextureName + "_TX"
 *  - Texture Coordinate Index: FeatureIDTextureName + "_UV_INDEX"
 *  - Channels: FeatureIDTextureName + "_CHANNELS"
 *  - NumChannels: FeatureIDTextureName + "_NUM_CHANNELS"
 */
static const FString MaterialTextureSuffix = "_TX";
static const FString MaterialTexCoordIndexSuffix = "_UV_INDEX";
static const FString MaterialChannelsSuffix = "_CHANNELS";
static const FString MaterialNumChannelsSuffix = "_NUM_CHANNELS";

/**
 * - Null Feature ID node: FeatureIDSetName + "_NULL_ID"
 */
static const FString MaterialNullFeatureIdSuffix = "_NULL_ID";

/**
 * Naming convention for metadata parameter nodes
 * - Property Table Property: "PTABLE_" + PropertyTableName + PropertyName
 */
static const FString MaterialPropertyTablePrefix = "PTABLE_";

/**
 * - Property Texture Property: "PTEXTURE_" + PropertyTextureName + PropertyName
 * - Property Texture Property UV Index: "PTEXTURE_" + PropertyTextureName +
 * PropertyName + "_UV_INDEX"
 * - Property Texture Property Channels: "PTEXTURE_" + PropertyTextureName +
 * PropertyName + "_CHANNELS"
 */
static const FString MaterialPropertyTexturePrefix = "PTEXTURE_";

/**
 * Below, "PropertyEntityName" represents the name of either a property table or
 * property texture.
 *
 * - Property Offset: Prefix + PropertyEntityName + PropertyName + "_OFFSET"
 * - Property Scale: Prefix + PropertyEntityName + PropertyName + "_SCALE"
 * - Property NoData: Prefix + PropertyEntityName + PropertyName + "_NO_DATA"
 * - Property Default Value: Prefix + PropertyEntityName + PropertyName +
 * "_DEFAULT"
 * - Property Has Value Qualifier: Prefix + PropertyEntityName + PropertyName
 * + "_HAS_VALUE"
 */
static const FString MaterialPropertyOffsetSuffix = "_OFFSET";
static const FString MaterialPropertyScaleSuffix = "_SCALE";
static const FString MaterialPropertyNoDataSuffix = "_NO_DATA";
static const FString MaterialPropertyDefaultValueSuffix = "_DEFAULT";
static const FString MaterialPropertyHasValueSuffix = "_HAS_VALUE";

/**
 * Naming convention for material inputs (for use in custom functions):
 * - Property Data: PropertyName + "_DATA"
 * - Property Raw Value: PropertyName + "_RAW"
 * - Property Transform Value: TransformName +
 * "_VALUE"
 * - Property UV Value: PropertyName = "_UV"
 */
static const FString MaterialPropertyDataSuffix = "_DATA";
static const FString MaterialPropertyRawSuffix = "_RAW";
static const FString MaterialPropertyValueSuffix = "_VALUE";
static const FString MaterialPropertyUVSuffix = "_UV";

/**
 * Naming convention for KHR_texture_transform inputs:
 *  - Texture Scale + Offset: TextureName + "_TX_SCALE_OFFSET"
 *  - Texture Rotation: TextureName + "_TX_ROTATION"
 */
static const FString MaterialTextureScaleOffsetSuffix = "_TX_SCALE_OFFSET";
static const FString MaterialTextureRotationSuffix = "_TX_ROTATION";

#pragma region Encoded Primitive Features

/**
 * @brief Generates a name for a feature ID set in a glTF primitive's
 * EXT_mesh_features. If the feature ID set already has a label, this will
 * return the label. Otherwise, if the feature ID set is unlabeled, a name
 * will be generated like so:
 *
 * - If the feature ID set is an attribute, this will appear as
 * "_FEATURE_ID_<index>", where <index> is the set index specified in
 * the attribute.
 * - If the feature ID set is a texture, this will appear as
 * "_FEATURE_ID_TEXTURE_<index>", where <index> increments with the number
 * of feature ID textures seen in an individual primitive.
 * - If the feature ID set is an implicit set, this will appear as
 * "_IMPLICIT_FEATURE_ID". Implicit feature ID sets don't vary in
 * definition, so any additional implicit feature ID sets across the
 * primitives are counted by this one.
 *
 * This is used by FCesiumFeatureIdSetDescription to display the names of
 * the feature ID sets across a tileset.
 *
 * @param FeatureIDSet The feature ID set
 * @param FeatureIDTextureCounter The counter representing how many feature
 * ID textures have been seen in the primitive thus far. Will be incremented
 * by this function if the given feature ID set is a texture.
 */
FString getNameForFeatureIDSet(
    const FCesiumFeatureIdSet& FeatureIDSet,
    int32& FeatureIDTextureCounter);
/**
 * @brief Generates a HLSL-safe name for a feature ID set. This is used by the
 * material for parameter nodes.
 *
 * @param FeatureIDSet The feature ID set
 * @param FeatureIDTextureCounter The counter representing how many feature ID
 * textures have been seen in the primitive thus far. Will be incremented by
 * this function if the given feature ID set is a texture.
 */
FString getMaterialNameForFeatureIDSet();

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

  /**
   * @brief The KHR_texture_transform extension on this feature ID texture, if
   * it exists.
   */
  std::optional<CesiumGltf::KhrTextureTransform> textureTransform;
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

  /**
   * A value that indicates that no feature is associated with the vertices or
   * texels that have this value.
   */
  std::optional<int64> nullFeatureId;
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

#pragma region Encoded Metadata

/**
 * @brief Generates a name for a property table in a glTF model's
 * EXT_structural_metadata. If the property table already has a name, this will
 * return the name. Otherwise, if the property table is unlabeled, its
 * corresponding class will be substituted.
 *
 * This is used by FCesiumPropertyTableDescription to display the names of
 * the property tables across a tileset.
 *
 * @param PropertyTable The property table
 */
FString getNameForPropertyTable(const FCesiumPropertyTable& PropertyTable);

/**
 * @brief Generates a name for a property texture in a glTF model's
 * EXT_structural_metadata. If the property texture already has a name, this
 * will return the name. Otherwise, if the property texture is unlabeled, its
 * corresponding class will be substituted.
 *
 * This is used by FCesiumPropertyTextureDescription to display the names of
 * the property textures across a tileset.
 *
 * @param PropertyTexture The property texture
 */
FString
getNameForPropertyTexture(const FCesiumPropertyTexture& PropertyTexture);

/**
 * @brief Generates an HLSL-safe name for a property table property in a glTF
 * model's EXT_structural_metadata. This is formatted like so:
 *
 * "PTABLE_<table name>_<property name>"
 *
 * This is used to name the texture parameter corresponding to this property in
 * the generated Unreal material.
 */
FString getMaterialNameForPropertyTableProperty(
    const FString& propertyTableName,
    const FString& propertyName);

/**
 * @brief Generates an HLSL-safe name for a property texture property in a glTF
 * model's EXT_structural_metadata. This is formatted like so:
 *
 * "PTEXTURE_<texture name>_<property name>"
 *
 * This is used to name the texture parameter corresponding to this property in
 * the generated Unreal material.
 */
FString getMaterialNameForPropertyTextureProperty(
    const FString& propertyTableName,
    const FString& propertyName);

/**
 * A property table property that has been encoded for access on the GPU.
 */
struct EncodedPropertyTableProperty {
  /**
   * @brief The name of the property table property.
   */
  FString name;

  /**
   * @brief The property table property values, encoded into a texture.
   */
  TUniquePtr<CesiumTextureUtility::LoadedTextureResult> pTexture;

  /**
   * @brief The type that the metadata will be encoded as.
   */
  ECesiumEncodedMetadataType type;

  /**
   * @brief The property table property's offset.
   */
  FCesiumMetadataValue offset;

  /**
   * @brief The property table property's scale.
   */
  FCesiumMetadataValue scale;

  /**
   * @brief The property table property's "no data" value.
   */
  FCesiumMetadataValue noData;

  /**
   * @brief The property table property's default value.
   */
  FCesiumMetadataValue defaultValue;
};

/**
 * A property table whose properties have been encoded for access on the GPU.
 */
struct EncodedPropertyTable {
  /**
   * @brief The name assigned to this property table. This will be used to
   * construct variable names in the generated Unreal material.
   */
  FString name;

  /**
   * @brief The encoded properties in this property table.
   */
  TArray<EncodedPropertyTableProperty> properties;
};

/**
 * A property texture property that has been made accessible to Unreal materials
 * through the GPU.
 */
struct EncodedPropertyTextureProperty {
  /**
   * @brief The name of the property texture property.
   */
  FString name;

  /**
   * @brief The texture used by the property texture property.
   */
  TSharedPtr<CesiumTextureUtility::LoadedTextureResult> pTexture;

  /**
   * @brief The type that of the metadata encoded in the texture.
   */
  ECesiumEncodedMetadataType type;

  /**
   * @brief The set index of the texture coordinates from the glTF primitive
   * that are used to sample this property texture. If this is -1, this texture
   * will not be sampled by texture coordinates in the primitive, but may be
   * sampled by other means in the Unreal material.
   */
  int64 textureCoordinateSetIndex;

  /**
   * @brief The channels to use when constructing a value from texture data. The
   * number of channels used is specified in the material itself, and derives
   * from the type of the property.
   */
  std::array<int32, 4> channels;

  /**
   * @brief The property table property's offset.
   */
  FCesiumMetadataValue offset;

  /**
   * @brief The property table property's scale.
   */
  FCesiumMetadataValue scale;

  /**
   * @brief The property table property's "no data" value.
   */
  FCesiumMetadataValue noData;

  /**
   * @brief The property table property's default value.
   */
  FCesiumMetadataValue defaultValue;

  /**
   * @brief The KHR_texture_transform extension on this feature ID texture, if
   * it exists.
   */
  std::optional<CesiumGltf::KhrTextureTransform> textureTransform;
};

/**
 * A property texture whose properties have been made accessible to Unreal
 * materials.
 */
struct EncodedPropertyTexture {
  /**
   * @brief The name assigned to this property texture. This will be used to
   * construct variable names in the generated Unreal material.
   */
  FString name;

  /**
   * @brief The encoded properties in this property texture.
   */
  TArray<EncodedPropertyTextureProperty> properties;
};

/**
 * @brief The encoded representation of the EXT_structural_metadata of a glTF
 * primitive.
 */
struct EncodedPrimitiveMetadata {
  /**
   * @brief The indices of the property textures used by the primitive.
   */
  TArray<int64> propertyTextureIndices;
};

/**
 * @brief The encoded representation of the EXT_structural_metadata of a glTF
 * model.
 */
struct EncodedModelMetadata {
  TArray<EncodedPropertyTable> propertyTables;
  TArray<EncodedPropertyTexture> propertyTextures;
};

EncodedPropertyTable encodePropertyTableAnyThreadPart(
    const FCesiumPropertyTableDescription& propertyTableDescription,
    const FCesiumPropertyTable& propertyTable);

EncodedPropertyTexture encodePropertyTextureAnyThreadPart(
    const FCesiumPropertyTextureDescription& propertyTextureDescription,
    const FCesiumPropertyTexture& propertyTexture,
    TMap<
        const CesiumGltf::ImageAsset*,
        TWeakPtr<CesiumTextureUtility::LoadedTextureResult>>&
        propertyTexturePropertyMap);

EncodedPrimitiveMetadata encodePrimitiveMetadataAnyThreadPart(
    const FCesiumPrimitiveMetadataDescription& metadataDescription,
    const FCesiumPrimitiveMetadata& primitive,
    const FCesiumModelMetadata& modelMetadata);

EncodedModelMetadata encodeModelMetadataAnyThreadPart(
    const FCesiumModelMetadataDescription& metadataDescription,
    const FCesiumModelMetadata& modelMetadata);

bool encodePropertyTableGameThreadPart(
    EncodedPropertyTable& encodedFeatureTable);

bool encodePropertyTextureGameThreadPart(
    TArray<TUniquePtr<CesiumTextureUtility::LoadedTextureResult>>&
        uniqueTextures,
    EncodedPropertyTexture& encodedFeatureTexture);

bool encodeModelMetadataGameThreadPart(EncodedModelMetadata& encodedMetadata);

void destroyEncodedModelMetadata(EncodedModelMetadata& encodedMetadata);

#pragma endregion

#pragma region Utility

struct EncodedPixelFormat {
  EPixelFormat format;
  int32_t bytesPerChannel;
  int32_t channels;
};

// TODO: consider picking better pixel formats when they are available for the
// current platform.
EncodedPixelFormat getPixelFormat(
    ECesiumEncodedMetadataType Type,
    ECesiumEncodedMetadataComponentType ComponentType);

FString createHlslSafeName(const FString& rawName);

bool isSupportedPropertyTextureProperty(
    const FCesiumMetadataPropertyDetails& PropertyDetails);

void SetPropertyParameterValue(
    UMaterialInstanceDynamic* pMaterial,
    EMaterialParameterAssociation association,
    int32 index,
    const FString& name,
    ECesiumEncodedMetadataType type,
    const FCesiumMetadataValue& value,
    float defaultValue);

void SetFeatureIdTextureParameterValues(
    UMaterialInstanceDynamic* pMaterial,
    EMaterialParameterAssociation association,
    int32 index,
    const FString& name,
    const EncodedFeatureIdTexture& encodedFeatureIdTexture);

void SetPropertyTableParameterValues(
    UMaterialInstanceDynamic* pMaterial,
    EMaterialParameterAssociation association,
    int32 index,
    const EncodedPropertyTable& encodedPropertyTable);

void SetPropertyTextureParameterValues(
    UMaterialInstanceDynamic* pMaterial,
    EMaterialParameterAssociation association,
    int32 index,
    const EncodedPropertyTexture& encodedPropertyTexture);

#pragma endregion

} // namespace CesiumEncodedFeaturesMetadata
