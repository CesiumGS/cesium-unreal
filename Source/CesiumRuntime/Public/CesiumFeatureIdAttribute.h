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
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureIdAttribute")
  static const FString&
  GetFeatureTableName(UPARAM(ref)
                          const FCesiumFeatureIdAttribute& FeatureIdAttribute);

  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureIdAttribute")
  static int64
  GetVertexCount(UPARAM(ref)
                     const FCesiumFeatureIdAttribute& FeatureIdAttribute);

  /**
   * Gets the feature ID associated with a given vertex.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureIdAttribute")
  static int64 GetFeatureIDForVertex(
      UPARAM(ref) const FCesiumFeatureIdAttribute& FeatureIdAttribute,
      int64 VertexIndex);
};
