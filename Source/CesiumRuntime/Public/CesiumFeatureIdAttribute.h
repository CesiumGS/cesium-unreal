// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumFeatureTable.h"
#include "CesiumGltf/AccessorView.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CesiumFeatureIdAttribute.generated.h"

namespace CesiumGltf {
struct Model;
struct Accessor;
struct FeatureTable;
} // namespace CesiumGltf

/**
 * @brief A blueprint-accessible wrapper for a feature ID attribute from a glTF
 * primitive. Provides access to per-vertex feature IDs which can be used with
 * the corresponding {@link FCesiumFeatureTable} to access per-vertex metadata.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumFeatureIdAttribute {
  GENERATED_USTRUCT_BODY()

  using FeatureIDAccessorType = std::variant<
      std::monostate,
      CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<int8_t>>,
      CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<uint8_t>>,
      CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<int16_t>>,
      CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<uint16_t>>,
      CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<uint32_t>>,
      CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<float>>>;

public:
  FCesiumFeatureIdAttribute() {}

  FCesiumFeatureIdAttribute(
      const CesiumGltf::Model& InModel,
      const CesiumGltf::MeshPrimitive Primitive,
      const int64 FeatureIDAttribute,
      const FString& PropertyTableName);

  int32 getAttributeIndex() const { return this->_attributeIndex; }

private:
  FeatureIDAccessorType _featureIDAccessor;
  int32 _attributeIndex;

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
           DeprecatedMessage =
               "UCesiumFeatureIdAttributeBlueprintLibrary.GetFeatureTableName is deprecated. Use UCesiumPrimitiveFeaturesBlueprintLibrary.GetPropertyTableIndex instead."))
  static const FString&
  GetFeatureTableName(UPARAM(ref)
                          const FCesiumFeatureIdAttribute& FeatureIDAttribute);
  PRAGMA_ENABLE_DEPRECATION_WARNINGS

  /**
   * @brief Get the number of vertices this primitive has.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|FeatureIDAttribute")
  static int64
  GetVertexCount(UPARAM(ref)
                     const FCesiumFeatureIdAttribute& FeatureIDAttribute);

  /**
   * Gets the feature ID associated with the given vertex. The feature ID can be
   * used with a FCesiumFeatureTable to retrieve the per-vertex
   * metadata.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|FeatureIDAttribute")
  static int64 GetFeatureIDForVertex(
      UPARAM(ref) const FCesiumFeatureIdAttribute& FeatureIDAttribute,
      int64 VertexIndex);
};
