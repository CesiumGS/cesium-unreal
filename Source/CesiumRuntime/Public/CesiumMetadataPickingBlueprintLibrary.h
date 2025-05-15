// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumMetadataValue.h"
#include "Containers/UnrealString.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ObjectMacros.h"
#include "CesiumMetadataPickingBlueprintLibrary.generated.h"

struct FHitResult;
struct FCesiumPrimitiveFeatures;
struct FCesiumModelMetadata;

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataPickingBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Compute the UV coordinates from the given line trace hit, assuming it has
   * hit a glTF primitive component that contains the specified texture
   * coordinate set. The texture coordinate set is specified relative to the
   * glTF itself, where the set index N resolves to the "TEXCOORD_N" attribute
   * in the glTF primitive.
   *
   * This function can be used to sample feature ID textures or property
   * textures in the primitive. This works similarly to the FindCollisionUV
   * Blueprint, except it does not require the texture coordinate sets to be
   * present in the model's physics mesh.
   *
   * Returns false if the given texture coordinate set index does not exist for
   * the primitive, or if its accessor is invalid.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Picking")
  static bool FindUVFromHit(
      const FHitResult& Hit,
      int64 GltfTexCoordSetIndex,
      FVector2D& UV);

  /**
   * Gets the property table values from a given line trace hit, assuming
   * that it has hit a feature of a glTF primitive component.
   *
   * A primitive may have multiple feature ID sets, so this allows a feature ID
   * set to be specified by index. This value should index into the array of
   * CesiumFeatureIdSets in the component's CesiumPrimitiveFeatures. If the
   * feature ID set is associated with a property table, it will return that
   * property table's data.
   *
   * For feature ID textures and implicit feature IDs, the feature ID can vary
   * across the face of a primitive. If the specified CesiumFeatureIdSet is one
   * of those types, the feature ID of the first vertex on the face will be
   * used.
   *
   * The returned result may be empty for several reasons:
   * - if the component is not a Cesium glTF primitive component
   * - if the hit's face index is somehow out-of-bounds
   * - if the specified feature ID set does not exist on the primitive
   * - if the specified feature ID set is not associated with a valid property
   * table
   *
   * Additionally, if any of the property table's properties are invalid, they
   * will not be included in the result.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Picking")
  static TMap<FString, FCesiumMetadataValue> GetPropertyTableValuesFromHit(
      const FHitResult& Hit,
      int64 FeatureIDSetIndex = 0);

  /**
   * Gets the property texture values from a given line trace hit, assuming it
   * has hit a glTF primitive component.
   *
   * A primitive may use multiple property textures, as indicated by its indices
   * in CesiumPrimitiveMetadata. This function allows for selection of which
   * property texture to use from those available in CesiumPrimitiveMetadata.
   *
   * In other words, the "Primitive Property Texture Index" should index into
   * the array property texture indices in the CesiumPrimitiveMetadata. The
   * primitive metadata will not necessarily contain all of the available
   * property textures in the CesiumModelMetadata, nor will it necessarily be
   * listed in the same order.
   *
   * The returned result may be empty for several reasons:
   * - if the component is not a Cesium glTF primitive component
   * - if the given primitive property texture index is out-of-bounds
   * - if the property texture index derived from CesiumPrimitiveMetadata
   * is out-of-bounds
   *
   * Additionally, if any of the property texture's properties are invalid, they
   * will not be included in the result.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Picking")
  static TMap<FString, FCesiumMetadataValue> GetPropertyTextureValuesFromHit(
      const FHitResult& Hit,
      int64 PrimitivePropertyTextureIndex = 0);

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  /**
   * Gets the metadata values for a face on a glTF primitive component.
   *
   * A primitive may have multiple feature ID sets, so this allows a feature ID
   * set to be specified by index. This value should index into the array of
   * CesiumFeatureIdSets in the component's CesiumPrimitiveFeatures. If the
   * feature ID set is associated with a property table, it will return that
   * property table's data.
   *
   * For feature ID textures and implicit feature IDs, the feature ID can vary
   * across the face of a primitive. If the specified CesiumFeatureIdSet is one
   * of those types, the feature ID of the first vertex on the face will be
   * used.
   *
   * The returned result may be empty for several reasons:
   * - if the component is not a Cesium glTF primitive component
   * - if the given face index is out-of-bounds
   * - if the specified feature ID set does not exist on the primitive
   * - if the specified feature ID set is not associated with a valid property
   * table
   *
   * Additionally, if any of the property table's properties are invalid, they
   * will not be included in the result.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Meta =
          (DeprecatedFunction,
           DeprecationMessage = "Use GetPropertyTableValuesFromHit instead."))
  static TMap<FString, FCesiumMetadataValue> GetMetadataValuesForFace(
      const UPrimitiveComponent* Component,
      int64 FaceIndex,
      int64 FeatureIDSetIndex = 0);

  /**
   * Gets the metadata values for a face on a glTF primitive component, as
   * strings.
   *
   * A primitive may have multiple feature ID sets, so this allows a feature ID
   * set to be specified by index. This value should index into the array of
   * CesiumFeatureIdSets in the component's CesiumPrimitiveFeatures. If the
   * feature ID set is associated with a property table, it will return that
   * property table's data.
   *
   * For feature ID textures and implicit feature IDs, the feature ID can vary
   * across the face of a primitive. If the specified CesiumFeatureIdSet is one
   * of those types, the feature ID of the first vertex on the face will be
   * used.
   *
   * The returned result may be empty for several reasons:
   * - if the component is not a Cesium glTF primitive component
   * - if the given face index is out-of-bounds
   * - if the specified feature ID set does not exist on the primitive
   * - if the specified feature ID set is not associated with a valid property
   * table
   *
   * Additionally, if any of the property table's properties are invalid, they
   * will not be included in the result. Array properties will return empty
   * strings.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "Use GetValuesAsStrings to convert the output of GetPropertyTableValuesFromHit instead."))
  static TMap<FString, FString> GetMetadataValuesForFaceAsStrings(
      const UPrimitiveComponent* Component,
      int64 FaceIndex,
      int64 FeatureIDSetIndex = 0);
  PRAGMA_ENABLE_DEPRECATION_WARNINGS

  /**
   * Retrieves a property table property from the component by name.
   * If the specified feature ID set does not exist or if the property table
   * does not contain a property with that name, the returned property will be
   * invalid.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Picking")
  static const FCesiumPropertyTableProperty& FindPropertyTableProperty(
      const UPrimitiveComponent* Component,
      const FString& PropertyName,
      int64 FeatureIDSetIndex = 0);

  /**
   * Retrieves a property table property by name, from the primitive features
   * and the model metadata.
   * If the specified feature ID set does not exist or if the property table
   * does not contain a property with that name, the returned property will be
   * invalid.
   */
  static const FCesiumPropertyTableProperty& FindPropertyTableProperty(
      const FCesiumPrimitiveFeatures& Features,
      const FCesiumModelMetadata& Metadata,
      const FString& PropertyName,
      int64 FeatureIDSetIndex);
};
