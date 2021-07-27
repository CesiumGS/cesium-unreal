// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/AccessorView.h"
#include "CesiumMetadataGenericValue.h"
#include "CesiumMetadataProperty.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ObjectMacros.h"
#include "CesiumMetadataFeatureTable.generated.h"

namespace CesiumGltf {
struct Model;
struct Accessor;
struct FeatureTable;
} // namespace CesiumGltf

/**
 * A Blueprint-accessible wrapper for a glTF feature table. A feature table is a
 * collection of properties for each feature ID in the mesh. It also knows how
 * to look up the feature ID associated with a given mesh vertex.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumMetadataFeatureTable {
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
  /**
   * Construct an empty feature table.
   */
  FCesiumMetadataFeatureTable(){};

  /**
   * Construct a feature table from a Gltf Feature Table.
   *
   * @param model The model that stores EXT_feature_metadata.
   * @param accessor The accessor of feature ID.
   * @param featureTable The feature table that is paired with feature ID.
   */
  FCesiumMetadataFeatureTable(
      const CesiumGltf::Model& model,
      const CesiumGltf::Accessor& accessor,
      const CesiumGltf::FeatureTable& featureTable);

  /**
   * Queries the number of features in the feature table.
   */
  int64 GetNumberOfFeatures() const;

  /**
   * Queries the feature ID associated with a given vertex.
   *
   * @param vertexIndex The index of the vertex.
   * @returns The feature ID, or -1 if no feature is associated with the vertex.
   */
  int64 GetFeatureIDForVertex(uint32 vertexIndex) const;

  /**
   * Returns a map of property name to property value for a given feature.
   *
   * @param featureID The ID of the feature.
   */
  TMap<FString, FCesiumMetadataGenericValue>
  GetPropertiesForFeatureID(int64 featureID) const;

  /**
   * Returns a map of property name to property value for a given feature, where
   * the value is converted to a string regardless of the underlying type.
   *
   * @param featureID The ID of the feature.
   */
  TMap<FString, FString>
  GetPropertiesAsStringsForFeatureID(int64 featureID) const;

  /**
   * Gets all the properties of the feature table.
   */
  const TMap<FString, FCesiumMetadataProperty>& GetProperties() const;

private:
  FeatureIDAccessorType _featureIDAccessor;
  TMap<FString, FCesiumMetadataProperty> _properties;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataFeatureTableBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Queries the number of features in the feature table.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureTable")
  static int64
  GetNumberOfFeatures(UPARAM(ref)
                          const FCesiumMetadataFeatureTable& featureTable);

  /**
   * Queries the feature ID associated with a given vertex.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureTable")
  static int64 GetFeatureIDForVertex(
      UPARAM(ref) const FCesiumMetadataFeatureTable& featureTable,
      int64 vertexIndex);

  /**
   * Returns a map of property name to property value for a given feature.
   *
   * @param featureID The ID of the feature.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureTable")
  static TMap<FString, FCesiumMetadataGenericValue> GetPropertiesForFeatureID(
      UPARAM(ref) const FCesiumMetadataFeatureTable& featureTable,
      int64 featureID);

  /**
   * Returns a map of property name to property value for a given feature, where
   * the value is converted to a string regardless of the underlying type.
   *
   * @param featureID The ID of the feature.
   */
  TMap<FString, FString> static GetPropertiesAsStringsForFeatureID(
      UPARAM(ref) const FCesiumMetadataFeatureTable& featureTable,
      int64 featureID);

  /**
   * Gets all the properties of the feature table.
   */
  static const TMap<FString, FCesiumMetadataProperty>&
  GetProperties(UPARAM(ref) const FCesiumMetadataFeatureTable& featureTable);
};
