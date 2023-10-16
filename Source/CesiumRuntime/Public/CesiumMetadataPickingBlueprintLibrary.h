// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumMetadataValue.h"
#include "Containers/UnrealString.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ObjectMacros.h"
#include "CesiumMetadataPickingBlueprintLibrary.generated.h"

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataPickingBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Gets the metadata values for a face on a glTF primitive component.
   *
   * A primitive may have multiple feature ID sets, so this allows a feature ID
   * set to be specified by index. This value should index into the array of
   * CesiumFeatureIdSets in the component's CesiumPrimitiveFeatures. If the
   * feature ID set is associated with a property table, that it will return
   * that property table's data.
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
      Category = "Cesium|Metadata|Picking")
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
   * feature ID set is associated with a property table, that it will return
   * that property table's data.
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
      Category = "Cesium|Metadata|Picking")
  static TMap<FString, FString> GetMetadataValuesForFaceAsStrings(
      const UPrimitiveComponent* Component,
      int64 FaceIndex,
      int64 FeatureIDSetIndex = 0);
};
