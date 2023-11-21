// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumFeatureIdSet.h"
#include "CesiumMetadataEncodingDetails.h"
#include "CesiumMetadataPropertyDetails.h"
#include "Components/ActorComponent.h"
#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "Misc/Guid.h"

#if WITH_EDITOR
#include "Materials/MaterialFunctionMaterialLayer.h"
#endif

#include "CesiumFeaturesMetadataComponent.generated.h"

#pragma region Features descriptions

/**
 * @brief Description of a feature ID set from EXT_mesh_features.
 */
USTRUCT()
struct CESIUMRUNTIME_API FCesiumFeatureIdSetDescription {
  GENERATED_USTRUCT_BODY()

  /**
   * The display name of the feature ID set. If the feature ID set already has a
   * label, this will use the label. Otherwise, if the feature ID set is
   * unlabeled, a name will be generated like so:
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
   * This name will also be used to represent the feature ID set in the
   * generated material.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  FString Name;

  /**
   * The type of the feature ID set.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  ECesiumFeatureIdSetType Type = ECesiumFeatureIdSetType::None;

  /**
   * The name of the property table that this feature ID set corresponds to.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  FString PropertyTableName;

  /**
   * The null feature ID for the feature ID set. This value indicates that no
   * feature is associated with the vertex or texel containing the value. If no
   * such value is specified, this defaults to -1, which prevents it from being
   * unnecessarily included in the generated material.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  bool bHasNullFeatureId = false;
};

/**
 * @brief Description of the feature ID sets available from the
 * EXT_mesh_features on a glTF's primitives.
 *
 * This aggregates the feature ID sets of all visible glTF primitives in the
 * model. This describes the feature IDs that can be made accessible
 * to Unreal Engine materials.
 */
USTRUCT()
struct CESIUMRUNTIME_API FCesiumPrimitiveFeaturesDescription {
  GENERATED_USTRUCT_BODY()

  /**
   * @brief The feature ID sets to make accessible to the material. Note that
   * the order of feature ID sets in this array does not necessarily
   * correspond to the order of these feature ID sets in a glTF
   * primitive.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Features",
      Meta = (TitleProperty = "Name"))
  TArray<FCesiumFeatureIdSetDescription> FeatureIdSets;
};
#pragma endregion

#pragma region Metadata descriptions

// These don't exhaustively cover the possibilities of glTF metadata
// classes, they only cover the subset that can be encoded into textures. The
// following types are excluded:
// - enums
// - strings that cannot be parsed as numbers or colors
// - matrices
// - variable length arrays
// - arrays of non-scalar, non-boolean elements
//
// Additionally, if a property contains fixed-length arrays, only the first four
// elements can be encoded.

/**
 * @brief Description of a property table property that should be encoded for
 * access on the GPU.
 */
USTRUCT()
struct CESIUMRUNTIME_API FCesiumPropertyTablePropertyDescription {
  GENERATED_USTRUCT_BODY()

  /**
   * The name of this property. This will be how it is referenced in the
   * material.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  FString Name;

  /**
   * Describes the underlying type of this property and other relevant
   * information from its EXT_structural_metadata definition. Not all types of
   * properties can be encoded to the GPU, or coerced to GPU-compatible types.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  FCesiumMetadataPropertyDetails PropertyDetails;

  /**
   * Describes how the property will be encoded as data on the GPU, if possible.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  FCesiumMetadataEncodingDetails EncodingDetails;
};

/**
 * @brief Description of a property table containing properties to be encoded
 * for access in Unreal materials.
 */
USTRUCT()
struct CESIUMRUNTIME_API FCesiumPropertyTableDescription {
  GENERATED_USTRUCT_BODY()

  /**
   * @brief The name of this property table. If this property table has no name
   * in the EXT_structural_metadata extension, then its class name is used
   * instead.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  FString Name;

  /**
   * @brief Descriptions of the properties to upload to the GPU.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium", Meta = (TitleProperty = "Name"))
  TArray<FCesiumPropertyTablePropertyDescription> Properties;
};

/**
 * @brief Description of a property texture property that should be made
 * accessible to Unreal materials. A property texture property's data is
 * already available through a texture, so no additional encoding details need
 * to be specified.
 */
USTRUCT()
struct CESIUMRUNTIME_API FCesiumPropertyTexturePropertyDescription {
  GENERATED_USTRUCT_BODY()

  /**
   * The name of this property. This will be how it is referenced in the
   * material.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  FString Name;

  /**
   * Describes the underlying type of this property and other relevant
   * information from its EXT_structural_metadata definition.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  FCesiumMetadataPropertyDetails PropertyDetails;
};

/**
 * @brief Description of a property texture with properties that should be
 * made accessible to Unreal materials.
 */
USTRUCT()
struct CESIUMRUNTIME_API FCesiumPropertyTextureDescription {
  GENERATED_USTRUCT_BODY()

  /**
   * @brief The name of this property texture.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  FString Name;

  /**
   * @brief Descriptions of the properties to upload to the GPU.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium", Meta = (TitleProperty = "Name"))
  TArray<FCesiumPropertyTexturePropertyDescription> Properties;
};

/**
 * @brief Names of the metadata entities referenced by the
 * EXT_structural_metadata on a glTF's primitives.
 *
 * This aggregates the metadata of all visible glTF primitives in the model.
 * This lists the names of the property textures actually used by the glTF
 * primitive, indicating it can be sampled with the primitive's texture
 * coordinates in the Unreal material.
 */
USTRUCT()
struct CESIUMRUNTIME_API FCesiumPrimitiveMetadataDescription {
  GENERATED_USTRUCT_BODY()

  /**
   * @brief The names of the property textures used by the glTF primitives
   * across the tileset.
   *
   * This should be a subset of the property textures listed in the model
   * metadata. Property textures can be passed to the material even if they are
   * not explicitly used by a glTF primitive, but the primitive may lack the
   * corresponding sets of texture coordinates intended to sample them.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Metadata",
      Meta = (TitleProperty = "Name"))
  TSet<FString> PropertyTextureNames;
};

/**
 * @brief Description of metadata from a glTF's EXT_structural_metadata
 * extension that should be uploaded to the GPU for access in Unreal materials.
 */
USTRUCT()
struct CESIUMRUNTIME_API FCesiumModelMetadataDescription {
  GENERATED_USTRUCT_BODY()

  /**
   * @brief Descriptions of property tables to encode for access in Unreal
   * materials.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Metadata",
      Meta = (TitleProperty = "Name"))
  TArray<FCesiumPropertyTableDescription> PropertyTables;

  /**
   * @brief Descriptions of property textures to make accessible to Unreal
   * materials.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Metadata",
      Meta = (TitleProperty = "Name"))
  TArray<FCesiumPropertyTextureDescription> PropertyTextures;
};

#pragma endregion

/**
 * @brief Description of both feature IDs and metadata from a glTF via the
 * EXT_mesh_Features and EXT_structural_metadata extensions. Indicates what
 * parts of the extension should be uploaded to the GPU for access in Unreal
 * materials.
 */
USTRUCT()
struct CESIUMRUNTIME_API FCesiumFeaturesMetadataDescription {
  GENERATED_USTRUCT_BODY()

public:
  /**
   * @brief Description of the feature ID sets available from the
   * EXT_mesh_features on a glTF's primitives.
   */
  FCesiumPrimitiveFeaturesDescription Features;

  /**
   * @brief Description of the metadata used by the EXT_structural_metadata on a
   * glTF's primitives.
   */
  FCesiumPrimitiveMetadataDescription PrimitiveMetadata;

  /**
   * @brief Description of metadata from a glTF's EXT_structural_metadata
   * extension.
   */
  FCesiumModelMetadataDescription ModelMetadata;
};

/**
 * @brief A component that can be added to Cesium3DTileset actors to
 * dictate what metadata to encode for access on the GPU. The selection can be
 * automatically populated based on available metadata by clicking the
 * "Auto Fill" button. Once a selection of desired metadata is made, the
 * boiler-plate material code to access the selected properties can be
 * auto-generated using the "Generate Material" button.
 */
UCLASS(ClassGroup = (Cesium), Meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumFeaturesMetadataComponent
    : public UActorComponent {
  GENERATED_BODY()

public:
#if WITH_EDITOR
  /**
   * Populate the description of metadata and feature IDs using the current view
   * of the tileset. This determines what to encode to the GPU based on the
   * existing metadata.
   *
   * Warning: Using Auto Fill may populate the description with a large amount
   * of metadata. Make sure to delete the properties that aren't relevant.
   */
  UFUNCTION(CallInEditor, Category = "Cesium")
  void AutoFill();

  /**
   * This button can be used to create a boiler-plate material layer that
   * exposes the requested metadata properties in the current description. The
   * nodes to access the metadata will be added to TargetMaterialLayer if it
   * exists. Otherwise a new material layer will be created in the /Content/
   * folder and TargetMaterialLayer will be set to the new material layer.
   */
  UFUNCTION(CallInEditor, Category = "Cesium")
  void GenerateMaterial();
#endif

#if WITH_EDITORONLY_DATA
  /**
   * This is the target UMaterialFunctionMaterialLayer that the
   * boiler-plate material generation will use. When pressing
   * "Generate Material", nodes will be added to this material to enable access
   * to the requested metadata. If this is left blank, a new material layer
   * will be created in the /Game/ folder.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  UMaterialFunctionMaterialLayer* TargetMaterialLayer = nullptr;
#endif

  // Using the FCesiumPrimitiveFeaturesDescription and
  // FCesiumModelMetadataDescription structs makes the UI less readable, so the
  // component uses arrays directly to help flatten the UI.

  /**
   * Description of the feature ID sets in the visible glTF primitives across
   * the tileset.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium|Primitive Features",
      Meta = (TitleProperty = "Name"))
  TArray<FCesiumFeatureIdSetDescription> FeatureIdSets;

  /**
   * Names of the property textures used by the glTF primitives across the
   * tileset.
   *
   * This should be a subset of the property textures listed in the model
   * metadata. Property textures can be passed to the material even if they are
   * not explicitly used by a glTF primitive, but the primitive may lack the
   * corresponding sets of texture coordinates intended to sample them.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium|Primitive Metadata",
      Meta = (TitleProperty = "Name"))
  TSet<FString> PropertyTextureNames;

  /**
   * Descriptions of the property tables in the visible glTF
   * models across the tileset.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium|Model Metadata",
      Meta = (TitleProperty = "Name"))
  TArray<FCesiumPropertyTableDescription> PropertyTables;

  /**
   * Descriptions of property textures in the visible glTF models across
   * the tileset.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium|Model Metadata",
      Meta = (TitleProperty = "Name"))
  TArray<FCesiumPropertyTextureDescription> PropertyTextures;
};
