// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumFeatureIdSet.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ObjectMacros.h"
#include <CesiumGltf/AccessorUtility.h>

#include "CesiumPrimitiveFeatures.generated.h"

namespace CesiumGltf {
struct ExtensionExtMeshFeatures;
struct ExtensionExtInstanceFeatures;
} // namespace CesiumGltf

/**
 * A Blueprint-accessible wrapper for a glTF Primitive's mesh features. It holds
 * views of the feature ID sets associated with this primitive. The collection
 * of features in the EXT_instance_features is very similar to that in
 * EXT_mesh_features, so FCesiumPrimitiveFeatures can be used to handle those
 * features too.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumPrimitiveFeatures {
  GENERATED_USTRUCT_BODY()

public:
  /**
   * Constructs an empty primitive features instance.
   */
  FCesiumPrimitiveFeatures() : _vertexCount(0) {}

  /**
   * Constructs a primitive features instance.
   *
   * @param Model The model that contains the EXT_mesh_features extension.
   * @param Primitive The mesh primitive that stores the EXT_mesh_features
   * extension
   * @param Features The EXT_mesh_features of the glTF mesh primitive
   */
  FCesiumPrimitiveFeatures(
      const CesiumGltf::Model& Model,
      const CesiumGltf::MeshPrimitive& Primitive,
      const CesiumGltf::ExtensionExtMeshFeatures& Features);

  /**
   * Constructs an instance feature object.
   *
   * @param Model The model that contains the EXT_instance_features extension
   * @param Node The node that stores the EXT_instance_features
   * extension
   * @param InstanceFeatures The EXT_instance_features of the glTF mesh
   * primitive
   */
  FCesiumPrimitiveFeatures(
      const CesiumGltf::Model& Model,
      const CesiumGltf::Node& Node,
      const CesiumGltf::ExtensionExtInstanceFeatures& InstanceFeatures);

private:
  TArray<FCesiumFeatureIdSet> _featureIdSets;
  CesiumGltf::IndexAccessorType _indexAccessor;
  // Vertex count = 0 and _primitiveMode = -1 indicates instance features
  int64_t _vertexCount;
  int32_t _primitiveMode;

  friend class UCesiumPrimitiveFeaturesBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumPrimitiveFeaturesBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Gets the primitive features of a glTF primitive component. If `component`
   * is not a Cesium glTF primitive component, the returned features are empty.
   *
   * @param component The component to get the features of.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|Features")
  static const FCesiumPrimitiveFeatures&
  GetPrimitiveFeatures(const UPrimitiveComponent* component);

  /**
   * Gets all the feature ID sets that are associated with the
   * primitive.
   *
   * @param PrimitiveFeatures The primitive.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|Features")
  static const TArray<FCesiumFeatureIdSet>&
  GetFeatureIDSets(UPARAM(ref)
                       const FCesiumPrimitiveFeatures& PrimitiveFeatures);

  /**
   * Gets all the feature ID sets of the given type.
   *
   * @param PrimitiveFeatures The primitive.
   * @param Type The type of feature ID set to obtain. If the primitive has no
   * sets of that type, the returned array will be empty.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|Features")
  static const TArray<FCesiumFeatureIdSet> GetFeatureIDSetsOfType(
      UPARAM(ref) const FCesiumPrimitiveFeatures& PrimitiveFeatures,
      ECesiumFeatureIdSetType Type);

  /**
   * Get the number of vertices in the primitive.
   *
   * @param PrimitiveFeatures The primitive.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|Features")
  static int64
  GetVertexCount(UPARAM(ref) const FCesiumPrimitiveFeatures& PrimitiveFeatures);

  /**
   * Gets the index of the first vertex that makes up a given face of this
   * primitive.
   *
   * @param PrimitiveFeatures The primitive.
   * @param FaceIndex The index of the face.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|Features")
  static int64 GetFirstVertexFromFace(
      UPARAM(ref) const FCesiumPrimitiveFeatures& PrimitiveFeatures,
      int64 FaceIndex);

  /**
   * Gets the feature ID associated with the given face.
   *
   * @param PrimitiveFeatures The primitive.
   * @param FaceIndex The index of the face to obtain the feature ID of.
   * @param FeatureIDSetIndex  A primitive may have multiple feature ID sets, so
   * this allows a feature ID set to be specified by index. This value should
   * index into the array of CesiumFeatureIdSets in the CesiumPrimitiveFeatures.
   * If the specified feature ID set index is invalid, this returns -1.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|Features")
  static int64 GetFeatureIDFromFace(
      UPARAM(ref) const FCesiumPrimitiveFeatures& PrimitiveFeatures,
      int64 FaceIndex,
      int64 FeatureIDSetIndex = 0);

  /**
   * Gets the feature ID associated with the instance at the given index.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|Features")
  static int64 GetFeatureIDFromInstance(
      UPARAM(ref) const FCesiumPrimitiveFeatures& InstanceFeatures,
      int64 InstanceIndex,
      int64 FeatureIDSetIndex = 0);

  /**
   * Gets the feature ID from the given line trace hit, assuming it
   * has hit a glTF primitive component containing this CesiumPrimitiveFeatures.
   *
   * @param PrimitiveFeatures The primitive.
   * @param Hit The line trace hit to try to obtain a feature ID from.
   * @param FeatureIDSetIndex A primitive may have multiple feature ID sets, so
   * this allows a feature ID set to be specified by index. This value should
   * index into the array of CesiumFeatureIdSets in the CesiumPrimitiveFeatures.
   * If the specified feature ID set index is invalid, this returns -1.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|Features")
  static int64 GetFeatureIDFromHit(
      UPARAM(ref) const FCesiumPrimitiveFeatures& PrimitiveFeatures,
      const FHitResult& Hit,
      int64 FeatureIDSetIndex = 0);
};
