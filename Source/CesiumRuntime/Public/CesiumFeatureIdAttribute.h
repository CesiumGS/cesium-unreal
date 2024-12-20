// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include <CesiumGltf/AccessorUtility.h>
#include "CesiumFeatureIdAttribute.generated.h"

namespace CesiumGltf {
struct Model;
struct Accessor;
struct Node;
} // namespace CesiumGltf

/**
 * @brief Reports the status of a FCesiumFeatureIdAttribute. If the feature ID
 * attribute cannot be accessed, this briefly indicates why.
 */
UENUM(BlueprintType)
enum class ECesiumFeatureIdAttributeStatus : uint8 {
  /* The feature ID attribute is valid. */
  Valid = 0,
  /* The feature ID attribute does not exist in the glTF primitive. */
  ErrorInvalidAttribute,
  /* The feature ID attribute uses an invalid accessor in the glTF. */
  ErrorInvalidAccessor
};

/**
 * @brief A blueprint-accessible wrapper for a feature ID attribute from a glTF
 * primitive. Provides access to per-vertex feature IDs which can be used with
 * the corresponding {@link FCesiumFeatureTable} to access per-vertex metadata.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumFeatureIdAttribute {
  GENERATED_USTRUCT_BODY()

public:
  /**
   * @brief Constructs an empty feature ID attribute instance. Empty feature ID
   * attributes can be constructed while trying to convert a FCesiumFeatureIdSet
   * that is not an attribute. In this case, the status reports it is an invalid
   * attribute.
   */
  FCesiumFeatureIdAttribute()
      : _status(ECesiumFeatureIdAttributeStatus::ErrorInvalidAttribute),
        _featureIdAccessor(),
        _attributeIndex(-1) {}

  /**
   * @brief Constructs a feature ID attribute instance.
   *
   * @param Model The model.
   * @param Primitive The mesh primitive containing the feature ID attribute.
   * @param FeatureIDAttribute The attribute index specified by the FeatureId.
   * @param PropertyTableName The name of the property table this attribute
   * corresponds to, if one exists, for backwards compatibility.
   */
  FCesiumFeatureIdAttribute(
      const CesiumGltf::Model& Model,
      const CesiumGltf::MeshPrimitive& Primitive,
      const int64 FeatureIDAttribute,
      const FString& PropertyTableName);

  /**
   * @brief Constructs a feature ID attribute instance from
   * EXT_instance_features data.
   *
   * @param Model The model.
   * @param Node The node containing the feature ID attribute.
   * @param FeatureIDAttribute The attribute index specified by the FeatureId.
   * @param PropertyTableName The name of the property table this attribute
   * corresponds to, if one exists, for backwards compatibility.
   */
  FCesiumFeatureIdAttribute(
      const CesiumGltf::Model& Model,
      const CesiumGltf::Node& Node,
      const int64 FeatureIDAttribute,
      const FString& PropertyTableName);

  /**
   * Gets the index of this feature ID attribute in the glTF primitive.
   */
  int64 getAttributeIndex() const { return this->_attributeIndex; }

private:
  ECesiumFeatureIdAttributeStatus _status;
  CesiumGltf::FeatureIdAccessorType _featureIdAccessor;
  int64 _attributeIndex;

  // For backwards compatibility.
  FString _propertyTableName;

  friend class UCesiumFeatureIdAttributeBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumFeatureIdAttributeBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  /**
   * Get the name of the feature table corresponding to this feature ID
   * attribute. The name can be used to fetch the appropriate
   * FCesiumFeatureTable from the FCesiumMetadataModel.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureIdAttribute",
      Meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "Use GetPropertyTableIndex on a CesiumFeatureIdSet instead."))
  static const FString&
  GetFeatureTableName(UPARAM(ref)
                          const FCesiumFeatureIdAttribute& FeatureIDAttribute);
  PRAGMA_ENABLE_DEPRECATION_WARNINGS

  /**
   * Gets the status of the feature ID attribute. If this attribute is
   * invalid in any way, this will briefly indicate why.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Features|FeatureIDAttribute")
  static ECesiumFeatureIdAttributeStatus GetFeatureIDAttributeStatus(
      UPARAM(ref) const FCesiumFeatureIdAttribute& FeatureIDAttribute);

  /**
   * Get the number of feature IDs in this attribute. If the feature ID
   * attribute is invalid, this returns 0.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Features|FeatureIDAttribute")
  static int64
  GetFeatureIDCount(UPARAM(ref)
                        const FCesiumFeatureIdAttribute& FeatureIDAttribute);

  /**
   * Gets the feature ID at the given index. A feature ID can be used with a
   * FCesiumPropertyTable to retrieve the metadata for that ID. If the feature
   * ID attribute is invalid, this returns -1.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Features|FeatureIDAttribute")
  static int64 GetFeatureID(
      UPARAM(ref) const FCesiumFeatureIdAttribute& FeatureIDAttribute,
      int64 Index);
};
