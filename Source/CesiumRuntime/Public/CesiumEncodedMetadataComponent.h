// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Components/ActorComponent.h"
#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "Misc/Guid.h"

#if WITH_EDITOR
#include "Materials/MaterialFunctionMaterialLayer.h"
#endif

#include "CesiumEncodedMetadataComponent.generated.h"

/**
 * @brief The GPU component type to coerce this property to.
 *
 */
UENUM()
enum class ECesiumPropertyComponentType : uint8 { Uint8, Float };

/**
 * @brief The property type.
 */
UENUM()
enum class ECesiumPropertyType : uint8 { Scalar, Vec2, Vec3, Vec4 };

/**
 * @brief Describes how this feature table is accessed. Either through feature
 * id textures, feature id attributes, mixed, or neither.
 */
UENUM()
enum class ECesiumFeatureTableAccessType : uint8 {
  Unknown,
  Texture,
  Attribute,
  Mixed
};

// Note that these don't exhaustively cover the possibilities of glTF metadata
// classes, they only cover the subset that can be encoded into textures. For
// example, arbitrary size arrays and enums are excluded. Other un-encoded
// types like strings will be coerced.

/**
 * @brief Description of a feature table property that should be encoded for
 * access on the GPU.
 */
USTRUCT()
struct CESIUMRUNTIME_API FPropertyDescription {
  GENERATED_USTRUCT_BODY()

  /**
   * @brief The name of this property as it will be referenced in the
   * material.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  FString Name;

  /**
   * @brief The GPU component type to coerce this property to.
   *
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  ECesiumPropertyComponentType ComponentType =
      ECesiumPropertyComponentType::Float;

  /**
   * @brief The property type.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  ECesiumPropertyType Type = ECesiumPropertyType::Scalar;

  /**
   * @brief If ComponentType==Uint8, this indicates whether to normalize into a
   * [0-1] range before accessing on the GPU.
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
 * @brief Description of a feature table containing properties to be encoded
 * for access on the GPU.
 */
USTRUCT()
struct CESIUMRUNTIME_API FFeatureTableDescription {
  GENERATED_USTRUCT_BODY()

  /**
   * @brief The name of this feature table.
   *
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  FString Name;

  /**
   * @brief Describes how this feature table is accessed. Either through feature
   * id textures, feature id attributes, mixed, or neither.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  ECesiumFeatureTableAccessType AccessType =
      ECesiumFeatureTableAccessType::Unknown;

  /**
   * @brief If the AccessType==Texture, this string represents the channel of
   * the feature id texture that will be used to index into this feature table.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      Meta =
          (EditCondition =
               "AccessType==ECesiumFeatureTableAccessType::Texture"))
  FString Channel;

  /**
   * @brief Descriptions of the properties to upload to the GPU.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium", Meta = (TitleProperty = "Name"))
  TArray<FPropertyDescription> Properties;
};

/**
 * @brief Description of a feature texture property that should be uploaded to
 * the GPU.
 */
USTRUCT()
struct CESIUMRUNTIME_API FFeatureTexturePropertyDescription {
  GENERATED_USTRUCT_BODY()

  /**
   * @brief The name of this property as it will be referenced in the
   * material.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  FString Name;

  // For now, always assumes it is Uint8
  /*
  UPROPERTY(EditAnywhere, Category = "Cesium")
  ECesiumPropertyComponentType ComponentType =
      ECesiumPropertyComponentType::Uint8;*/

  /**
   * @brief The property type.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  ECesiumPropertyType Type = ECesiumPropertyType::Scalar;

  /**
   * @brief If ComponentType==Uint8, this indicates whether to normalize into a
   * [0-1] range before accessing on the GPU.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  bool Normalized = false;

  /**
   * @brief This string describes the channel order of the incoming feature
   * texture property (e.g., "rgb", "bgra", etc.). This helps us fix the
   * channel order when accessing on the GPU.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  FString Swizzle;
};

/**
 * @brief Description of a feature texture with properties that should be
 * uploaded to the GPU.
 */
USTRUCT()
struct CESIUMRUNTIME_API FFeatureTextureDescription {
  GENERATED_USTRUCT_BODY()

  /**
   * @brief The name of this feature texture.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  FString Name;

  /**
   * @brief Descriptions of the properties to upload to the GPU.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium", Meta = (TitleProperty = "Name"))
  TArray<FFeatureTexturePropertyDescription> Properties;
};

/**
 * @brief Description of metadata from a glTF that should be uploaded to the
 * GPU for access in materials.
 */
USTRUCT()
struct CESIUMRUNTIME_API FMetadataDescription {
  GENERATED_USTRUCT_BODY()

  /**
   * @brief Descriptions of feature tables to upload to the GPU.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "EncodeMetadata",
      Meta = (TitleProperty = "Name"))
  TArray<FFeatureTableDescription> FeatureTables;

  /**
   * @brief Descriptions of feature textures to upload to the GPU.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "EncodeMetadata",
      Meta = (TitleProperty = "Name"))
  TArray<FFeatureTextureDescription> FeatureTextures;
};

/**
 * @brief An actor component that can be added to Cesium3DTileset actors to
 * dictate what metadata to encode for access on the GPU. The selection can be
 * automatically populated based on available metadata by clicking the
 * "Auto Fill" button. Once a selection of desired metadata is made, the
 * boiler-plate material code to access the selected properties can be
 * auto-generated using the "Generate Material" button.
 */
UCLASS(ClassGroup = (Cesium), Meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API UCesiumEncodedMetadataComponent
    : public UActorComponent {
  GENERATED_BODY()

public:
  /**
   * @brief This button can be used to pre-populate the description of metadata
   * to encode based on the existing metadata in the current tiles.
   *
   * Warning: Using Auto Fill may populate the description with a large amount
   * of metadata. Make sure to delete the properties that aren't relevant.
   */
  UFUNCTION(CallInEditor, Category = "EncodeMetadata")
  void AutoFill();

#if WITH_EDITOR
  /**
   * @brief This button can be used to create a boiler-plate material layer that
   * exposes the requested metadata properties in the current description. The
   * nodes to access the metada will be added to TargetMaterialLayer if it
   * exists. Otherwise a new material layer will be created in the /Game/
   * folder and TargetMaterialLayer will be set to the new material layer.
   */
  UFUNCTION(CallInEditor, Category = "EncodeMetadata")
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
  UPROPERTY(EditAnywhere, Category = "EncodeMetadata")
  UMaterialFunctionMaterialLayer* TargetMaterialLayer = nullptr;
#endif

  // Note: Here, we avoid wrapping the feature tables and feature textures
  // inside a FMetadataDescription to avoid further complicating the details
  // panel UI for editing the hierarchy.

  /**
   * @brief Descriptions of feature tables to upload to the GPU.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "EncodeMetadata",
      Meta = (TitleProperty = "Name"))
  TArray<FFeatureTableDescription> FeatureTables;

  /**
   * @brief Descriptions of feature textures to upload to the GPU.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "EncodeMetadata",
      Meta = (TitleProperty = "Name"))
  TArray<FFeatureTextureDescription> FeatureTextures;
};
