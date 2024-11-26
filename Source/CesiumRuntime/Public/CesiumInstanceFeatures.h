// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumFeatureIdSet.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ObjectMacros.h"
#include <CesiumGltf/AccessorUtility.h>

#include "CesiumInstanceFeatures.generated.h"

namespace CesiumGltf {
struct ExtensionExtMeshFeatures;
} // namespace CesiumGltf

/**
 * A Blueprint-accessible wrapper for a glTF instance features. It holds
 * views of the feature ID sets associated with an instanced node.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumInstanceFeatures {
  GENERATED_USTRUCT_BODY()

public:
  /**
   * Constructs an empty primitive features instance.
   */
  FCesiumInstanceFeatures() : _instanceCount(0) {}

  /**
   * Constructs an instance feature.
   *
   * @param Model The model that contains the EXT_instance_features extension
   * @param Node The node that stores instance data
   * extension
   */
  FCesiumInstanceFeatures(
      const CesiumGltf::Model& Model,
      const CesiumGltf::Node& Node);

private:
  TArray<FCesiumFeatureIdSet> _featureIdSets;
  int64_t _instanceCount;

  friend class UCesiumInstanceFeaturesBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumInstanceFeaturesBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Gets the instance features of a glTF primitive component. If component is
   * not a Cesium glTF primitive component, the returned features are empty.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|Features")
  static const FCesiumInstanceFeatures&
  GetInstanceFeatures(const UPrimitiveComponent* component);

  /**
   * Gets all the feature ID sets that are associated with the
   * primitive.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|Features")
  static const TArray<FCesiumFeatureIdSet>&
  GetFeatureIDSets(UPARAM(ref) const FCesiumInstanceFeatures& InstanceFeatures);

  /**
   * Gets all the feature ID sets of the given type. If the primitive has no
   * sets of that type, the returned array will be empty.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|Features")
  static const TArray<FCesiumFeatureIdSet> GetFeatureIDSetsOfType(
      UPARAM(ref) const FCesiumInstanceFeatures& InstanceFeatures,
      ECesiumFeatureIdSetType Type);

  /**
   * Gets the feature ID associated with an instance number
   *
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|Features")
  static int64 GetFeatureIDFromInstance(
      UPARAM(ref) const FCesiumInstanceFeatures& InstanceFeatures,
      int64 InstanceIndex,
      int64 FeatureIDSetIndex = 0);
};
