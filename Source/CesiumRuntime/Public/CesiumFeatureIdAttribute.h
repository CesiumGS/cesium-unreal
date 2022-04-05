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
 * @brief A blueprint-accessible wrapper for a feature id attribute from a glTF
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
      const CesiumGltf::Model& Model,
      const CesiumGltf::Accessor& FeatureIDAccessor,
      const int32 attributeIndex,
      const FString& FeatureTableName);

  int32 getAttributeIndex() const { return this->_attributeIndex; }

private:
  FeatureIDAccessorType _featureIDAccessor;
  FString _featureTableName;
  int32 _attributeIndex;

  friend class UCesiumFeatureIdAttributeBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumFeatureIdAttributeBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()
public:
  /**
   * @brief Get the name of the feature table corresponding to this feature ID
   * attribute. The name can be used to fetch the appropriate
   * {@link FCesiumFeatureTable} from the {@link FCesiumMetadataModel}.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureIdAttribute")
  static const FString&
  GetFeatureTableName(UPARAM(ref)
                          const FCesiumFeatureIdAttribute& FeatureIdAttribute);

  /**
   * @brief Get the number of vertices this primitive has.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureIdAttribute")
  static int64
  GetVertexCount(UPARAM(ref)
                     const FCesiumFeatureIdAttribute& FeatureIdAttribute);

  /**
   * Gets the feature ID associated with a given vertex. The feature ID can be
   * used with a {@link FCesiumFeatureTable} to retrieve the per-vertex
   * metadata.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureIdAttribute")
  static int64 GetFeatureIDForVertex(
      UPARAM(ref) const FCesiumFeatureIdAttribute& FeatureIdAttribute,
      int64 VertexIndex);
};
