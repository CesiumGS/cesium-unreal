// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/AccessorView.h"
#include "CesiumMetadataFeatureTable.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CesiumVertexMetadata.generated.h"

namespace CesiumGltf {
struct Model;
struct Accessor;
struct FeatureTable;
} // namespace CesiumGltf

USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumVertexMetadata {
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
  FCesiumVertexMetadata() {}

  FCesiumVertexMetadata(
      const CesiumGltf::Model& Model,
      const CesiumGltf::Accessor& FeatureIDAccessor,
      const int32 attributeIndex,
      const FString& FeatureTableName);

  int32 getAttributeIndex() const { return this->_attributeIndex; }

private:
  FeatureIDAccessorType _featureIDAccessor;
  FString _featureTableName;
  int32 _attributeIndex;

  friend class UCesiumVertexMetadataBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumVertexMetadataBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()
public:
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|VertexMetadata")
  static const FString&
  GetFeatureTableName(UPARAM(ref) const FCesiumVertexMetadata& VertexMetadata);

  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|VertexMetadata")
  static int64 GetVertexCount(UPARAM(ref)
                                  const FCesiumVertexMetadata& VertexMetadata);

  /**
   * Gets the feature ID associated with a given vertex.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|VertexMetadata")
  static int64 GetFeatureIDForVertex(
      UPARAM(ref) const FCesiumVertexMetadata& VertexMetadata,
      int64 VertexIndex);
};