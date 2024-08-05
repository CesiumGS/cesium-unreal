// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumFeatureIdAttribute.h"
#include "CesiumFeatureIdTexture.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include <variant>
#include "CesiumFeatureIdSet.generated.h"

namespace CesiumGltf {
struct Model;
struct FeatureId;
struct ExtensionExtInstanceFeaturesFeatureId;
} // namespace CesiumGltf

/**
 * @brief The type of a feature ID set.
 */
UENUM(BlueprintType)
enum class ECesiumFeatureIdSetType : uint8 {
  None,
  Attribute,
  Texture,
  Implicit,
  Instance,
  InstanceImplicit
};

/**
 * @brief A blueprint-accessible wrapper for a feature ID set from a glTF
 * primitive. A feature ID can be defined as a per-vertex attribute, as a
 * feature texture, or implicitly via vertex ID. These can be used with the
 * corresponding {@link FCesiumPropertyTable} to access per-vertex metadata.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumFeatureIdSet {
  GENERATED_USTRUCT_BODY()

  using FeatureIDType = std::variant<
      std::monostate,
      FCesiumFeatureIdAttribute,
      FCesiumFeatureIdTexture>;

public:
  FCesiumFeatureIdSet()
      : _featureIDSetType(ECesiumFeatureIdSetType::None),
        _featureCount(0),
        _nullFeatureID(-1) {}

  FCesiumFeatureIdSet(
      const CesiumGltf::Model& Model,
      const CesiumGltf::MeshPrimitive& Primitive,
      const CesiumGltf::FeatureId& FeatureId);

  FCesiumFeatureIdSet(
      const CesiumGltf::Model& Model,
      const CesiumGltf::Node& Node,
      const CesiumGltf::ExtensionExtInstanceFeaturesFeatureId&
          InstanceFeatureId);

private:
  FeatureIDType _featureID;
  ECesiumFeatureIdSetType _featureIDSetType;
  int64 _featureCount;
  int64 _nullFeatureID;
  int64 _propertyTableIndex;
  FString _label;

  friend class UCesiumFeatureIdSetBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumFeatureIdSetBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()
public:
  /**
   * Gets the type of this feature ID set.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Features|FeatureIDSet")
  static const ECesiumFeatureIdSetType
  GetFeatureIDSetType(UPARAM(ref) const FCesiumFeatureIdSet& FeatureIDSet);

  /**
   * Gets this feature ID set as a feature ID attribute. This can be used for
   * more fine-grained interaction with the attribute itself. If this feature ID
   * is not defined as an attribute, then the returned attribute will be
   * invalid.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Features|FeatureIDSet")
  static const FCesiumFeatureIdAttribute&
  GetAsFeatureIDAttribute(UPARAM(ref) const FCesiumFeatureIdSet& FeatureIDSet);

  /**
   * Gets this feature ID set as a feature ID texture. This can be used for more
   * fine-grained interaction with the texture itself. If this feature ID is
   * not defined as a texture, then the returned texture will be invalid.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Features|FeatureIDSet")
  static const FCesiumFeatureIdTexture&
  GetAsFeatureIDTexture(UPARAM(ref) const FCesiumFeatureIdSet& FeatureIDSet);

  /**
   * Get the index of the property table corresponding to this feature
   * ID set. The index can be used to fetch the appropriate
   * FCesiumPropertyTable from the FCesiumModelMetadata. If the
   * feature ID set does not specify a property table, this returns -1.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Features|FeatureIDSet")
  static const int64
  GetPropertyTableIndex(UPARAM(ref) const FCesiumFeatureIdSet& FeatureIDSet);

  /**
   * Get the number of features this primitive has.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Features|FeatureIDSet")
  static int64 GetFeatureCount(UPARAM(ref)
                                   const FCesiumFeatureIdSet& FeatureIDSet);

  /**
   * Gets the null feature ID, i.e., the value that indicates no feature is
   * associated with the owner. In other words, if a vertex or texel returns
   * this value, then it is not associated with any feature.
   *
   * If this value was not defined in the glTF feature ID set, this defaults to
   * -1.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Features|FeatureIDSet")
  static const int64
  GetNullFeatureID(UPARAM(ref) const FCesiumFeatureIdSet& FeatureIDSet);

  /**
   * Gets the label assigned to this feature ID set. If no label was present in
   * the glTF feature ID set, this returns an empty string.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Features|FeatureIDSet")
  static const FString GetLabel(UPARAM(ref)
                                    const FCesiumFeatureIdSet& FeatureIDSet);

  /**
   * Gets the feature ID associated with a given vertex. The feature ID can be
   * used with a FCesiumPropertyTable to retrieve the corresponding metadata.
   *
   * This returns -1 if the given vertex is out-of-bounds, or if the feature ID
   * set is invalid (e.g., it contains an invalid feature ID texture).
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Features|FeatureIDSet")
  static int64 GetFeatureIDForVertex(
      UPARAM(ref) const FCesiumFeatureIdSet& FeatureIDSet,
      int64 VertexIndex);

  /**
   * Gets the feature ID associated with a given instance. The feature ID can be
   * used with a FCesiumPropertyTable to retrieve the corresponding metadata.
   *
   * This returns -1 if the given instance is out-of-bounds, if the feature ID
   * set is not for instances, or if the feature ID set is invalid (e.g., it
   * contains an invalid feature ID texture).
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Features|FeatureIDSet")
  static int64 GetFeatureIDForInstance(
      UPARAM(ref) const FCesiumFeatureIdSet& FeatureIDSet,
      int64 InstanceIndex);

  /**
   * Given a trace hit result, gets the feature ID from the feature ID set on
   * the hit component. This returns a more accurate value for feature ID
   * textures, since they define feature IDs per-texel instead of per-vertex.
   * The feature ID can be used with a FCesiumPropertyTable to retrieve the
   * corresponding metadata.
   *
   * This can still retrieve the feature IDs for non-texture feature ID sets.
   * For attribute or implicit feature IDs, the first feature ID associated
   * with the first vertex of the intersected face is returned.
   *
   * This returns -1 if the feature ID set is invalid (e.g., it contains an
   * invalid feature ID texture).
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Features|FeatureIDSet")
  static int64 GetFeatureIDFromHit(
      UPARAM(ref) const FCesiumFeatureIdSet& FeatureIDSet,
      const FHitResult& Hit);
};
