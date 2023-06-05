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
struct CESIUMRUNTIME_API FCesiumFeatureIDAttribute {
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
  FCesiumFeatureIDAttribute() {}

  FCesiumFeatureIDAttribute(
      const CesiumGltf::Model& InModel,
      const CesiumGltf::MeshPrimitive Primitive,
      const int64 FeatureIDAttribute);

  int32 getAttributeIndex() const { return this->_attributeIndex; }

private:
  FeatureIDAccessorType _featureIDAccessor;
  int32 _attributeIndex;

  friend class UCesiumFeatureIDAttributeBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumFeatureIDAttributeBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()
public:
  /**
   * @brief Get the number of vertices this primitive has.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|FeatureIDAttribute")
  static int64
  GetVertexCount(UPARAM(ref)
                     const FCesiumFeatureIDAttribute& FeatureIDAttribute);

  /**
   * Gets the feature ID associated with the given vertex. The feature ID can be
   * used with a {@link FCesiumFeatureTable} to retrieve the per-vertex
   * metadata.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Primitive|FeatureIDAttribute")
  static int64 GetFeatureIDForVertex(
      UPARAM(ref) const FCesiumFeatureIDAttribute& FeatureIDAttribute,
      int64 VertexIndex);
};
