// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumMetadataGenericValue.h"
#include "CesiumMetadataProperty.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ObjectMacros.h"
#include "CesiumMetadataFeatureTable.generated.h"

namespace CesiumGltf {
struct Model;
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

public:
  /**
   * Construct an empty feature table.
   */
  FCesiumMetadataFeatureTable(){};

  /**
   * Constructs a feature table from a glTF Feature Table.
   *
   * @param Model The model that stores EXT_feature_metadata.
   * @param FeatureTable The feature table that is paired with the feature ID.
   */
  FCesiumMetadataFeatureTable(
      const CesiumGltf::Model& Model,
      const CesiumGltf::FeatureTable& FeatureTable);

private:
  TMap<FString, FCesiumMetadataProperty> _properties;

  friend class UCesiumMetadataFeatureTableBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataFeatureTableBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Gets the number of features in the feature table.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureTable")
  static int64
  GetNumberOfFeatures(UPARAM(ref)
                          const FCesiumMetadataFeatureTable& FeatureTable);

  /**
   * Gets a map of property name to property value for a given feature.
   *
   * @param featureID The ID of the feature.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureTable")
  static TMap<FString, FCesiumMetadataGenericValue>
  GetMetadataValuesForFeatureID(
      UPARAM(ref) const FCesiumMetadataFeatureTable& FeatureTable,
      int64 FeatureID);

  /**
   * Gets a map of property name to property value for a given feature, where
   * the value is converted to a string regardless of the underlying type.
   *
   * @param FeatureID The ID of the feature.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureTable")
  static TMap<FString, FString> GetMetadataValuesAsStringForFeatureID(
      UPARAM(ref) const FCesiumMetadataFeatureTable& featureTable,
      int64 FeatureID);

  /**
   * Gets all the properties of the feature table.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureTable")
  static const TMap<FString, FCesiumMetadataProperty>&
  GetProperties(UPARAM(ref) const FCesiumMetadataFeatureTable& FeatureTable);
};
