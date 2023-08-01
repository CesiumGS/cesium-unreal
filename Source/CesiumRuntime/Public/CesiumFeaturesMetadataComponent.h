// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#pragma once

#include "Components/ActorComponent.h"
#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "Misc/Guid.h"

#if WITH_EDITOR
#include "Materials/MaterialFunctionMaterialLayer.h"
#endif

#include "CesiumFeaturesMetadataComponent.generated.h"

#pragma region Features descriptions

enum class ECesiumFeatureIdSetType : uint8;

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
  ECesiumFeatureIdSetType Type;

  /**
   * The name of the property table that this feature ID set corresponds to.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  FString PropertyTableName;
};

/**
 * @brief Description of the feature ID sets available from the
 * EXT_mesh_features on a glTF's primitives.
 *
 * This aggregates the feature ID sets of all visible glTF primitives in the
 * feature ID sets. This describes the feature IDs that can be made accessible
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
/**
 * @brief The GPU component type to coerce this property to.
 *
 */
UENUM()
enum class ECesiumEncodedPropertyComponentType : uint8 { Uint8, Float };

/**
 * @brief The property type.
 */
UENUM()
enum class ECesiumEncodedPropertyType : uint8 { Scalar, Vec2, Vec3, Vec4 };

// Note that these don't exhaustively cover the possibilities of glTF metadata
// classes, they only cover the subset that can be encoded into textures. For
// example, arbitrary size arrays and enums are excluded. Other un-encoded
// types like strings will be coerced.
// ^ this isn't true? strings don't seem to be handled

/**
 * @brief Description of a property table property that should be encoded for
 * access on the GPU.
 */
USTRUCT()
struct CESIUMRUNTIME_API FCesiumPropertyTablePropertyDescription {
  GENERATED_USTRUCT_BODY()

  /**
   * @brief The name of this property. This will be how it is referenced in the
   * material.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  FString Name;

  /**
   * @brief The GPU component type to coerce this property to.
   *
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  ECesiumEncodedPropertyComponentType ComponentType =
      ECesiumEncodedPropertyComponentType::Float;

  /**
   * @brief The property type.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  ECesiumEncodedPropertyType Type = ECesiumEncodedPropertyType::Scalar;

  /**
   * @brief If ComponentType == Uint8, this indicates whether to normalize into
   * a [0-1] range before accessing on the GPU.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      Meta =
          (EditCondition =
               "ComponentType==ECesiumPropertyComponentType::Uint8"))
  bool Normalized = false;
};

/**
 * @brief Description of a property table containing properties to be encoded
 * for access on the GPU.
 */
USTRUCT()
struct CESIUMRUNTIME_API FCesiumPropertyTableDescription {
  GENERATED_USTRUCT_BODY()

  /**
   * @brief The name of this feature table.
   *
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  FString Name;

  /**
   * @brief Descriptions of the properties to upload to the GPU.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium", Meta = (TitleProperty = "Name"))
  TArray<FCesiumPropertyTablePropertyDescription> Properties;
};

///**
// * @brief Description of a feature texture property that should be uploaded to
// * the GPU.
// */
// USTRUCT()
// struct CESIUMRUNTIME_API FFeatureTexturePropertyDescription {
//  GENERATED_USTRUCT_BODY()
//
//  /**
//   * @brief The name of this property as it will be referenced in the
//   * material.
//   */
//  UPROPERTY(EditAnywhere, Category = "Cesium")
//  FString Name;
//
//  // For now, always assumes it is Uint8
//  /*
//  UPROPERTY(EditAnywhere, Category = "Cesium")
//  ECesiumPropertyComponentType ComponentType =
//      ECesiumPropertyComponentType::Uint8;*/
//
//  /**
//   * @brief The property type.
//   */
//  UPROPERTY(EditAnywhere, Category = "Cesium")
//  ECesiumPropertyType Type = ECesiumPropertyType::Scalar;
//
//  /**
//   * @brief If ComponentType==Uint8, this indicates whether to normalize into
//   a
//   * [0-1] range before accessing on the GPU.
//   */
//  UPROPERTY(EditAnywhere, Category = "Cesium")
//  bool Normalized = false;
//
//  /**
//   * @brief This string describes the channel order of the incoming feature
//   * texture property (e.g., "rgb", "bgra", etc.). This helps us fix the
//   * channel order when accessing on the GPU.
//   */
//  UPROPERTY(EditAnywhere, Category = "Cesium")
//  FString Swizzle;
//};
//
///**
// * @brief Description of a feature texture with properties that should be
// * uploaded to the GPU.
// */
// USTRUCT()
// struct CESIUMRUNTIME_API FFeatureTextureDescription {
//  GENERATED_USTRUCT_BODY()
//
//  /**
//   * @brief The name of this feature texture.
//   */
//  UPROPERTY(EditAnywhere, Category = "Cesium")
//  FString Name;
//
//  /**
//   * @brief Descriptions of the properties to upload to the GPU.
//   */
//  UPROPERTY(EditAnywhere, Category = "Cesium", Meta = (TitleProperty =
//  "Name")) TArray<FFeatureTexturePropertyDescription> Properties;
//};

/**
 * @brief Description of metadata from a glTF's EXT_structural_metadata
 * extension that should be uploaded to the GPU for access in Unreal materials.
 */
USTRUCT()
struct CESIUMRUNTIME_API FCesiumModelMetadataDescription {
  GENERATED_USTRUCT_BODY()

  /**
   * @brief Descriptions of feature tables to upload to the GPU.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Metadata",
      Meta = (TitleProperty = "Name"))
  TArray<FCesiumPropertyTableDescription> PropertyTables;

  ///**
  // * @brief Descriptions of feature textures to upload to the GPU.
  // */
  // UPROPERTY(
  //    EditAnywhere,
  //    Category = "Metadata",
  //    Meta = (TitleProperty = "Name"))
  // TArray<FFeatureTextureDescription> PropertyTextures;
};

#pragma endregion

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

#if WITH_EDITOR
  /**
   * @brief This button can be used to create a boiler-plate material layer that
   * exposes the requested metadata properties in the current description. The
   * nodes to access the metadata will be added to TargetMaterialLayer if it
   * exists. Otherwise a new material layer will be created in the /Game/
   * folder and TargetMaterialLayer will be set to the new material layer.
   */
  UFUNCTION(CallInEditor, Category = "Cesium")
  void GenerateMaterial();
#endif

#if WITH_EDITORONLY_DATA
  /**
   * @brief This is the target UMaterialFunctionMaterialLayer that the
   * boiler-plate material generation will use. When pressing
   * "Generate Material", nodes will be added to this material to enable access
   * to the requested metadata. If this is left blank, a new material layer
   * will be created in the /Game/ folder.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium|EncodedMetadata")
  UMaterialFunctionMaterialLayer* TargetMaterialLayer = nullptr;
#endif

  // Note: Here, we avoid wrapping the feature tables and feature textures
  // inside a FMetadataDescription to avoid further complicating the details
  // panel UI for editing the hierarchy.

  /**
   * Description of the feature ID sets available from the
   * EXT_mesh_features on a glTF's primitives.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium|Features",
      Meta = (TitleProperty = "Name"))
  TArray<FCesiumFeatureIdSetDescription> FeatureIdSets;

  /**
   * @brief Descriptions of property tables to upload to the GPU.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium|Metadata",
      Meta = (TitleProperty = "Name"))
  TArray<FCesiumPropertyTableDescription> PropertyTables;

  ///**
  // * @brief Descriptions of feature textures to upload to the GPU.
  // */
  // UPROPERTY(
  //    EditAnywhere,
  //    Category = "Cesium|Metadata",
  //    Meta = (TitleProperty = "Name"))
  // TArray<FFeatureTextureDescription> FeatureTextures;
};
