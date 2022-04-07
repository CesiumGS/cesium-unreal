// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/AccessorView.h"
#include "CesiumMetadataGenericValue.h"
#include "CesiumMetadataProperty.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ObjectMacros.h"
#include "CesiumMetadataFeatureTable.generated.h"

namespace CesiumGltf {
struct Model;
struct Accessor;
struct FeatureTable;
} // namespace CesiumGltf

struct UE_DEPRECATED(
    4.26,
    "FCesiumMetadataFeatureTable is deprecated, use FCesiumFeatureTable instead.")
    FCesiumMetadataFeatureTable;

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
   * Constructs a feature table from a glTF Feature Table.
   *
   * @param Model The model that stores EXT_feature_metadata.
   * @param Accessor The accessor for the feature ID.
   * @param FeatureTable The feature table that is paired with the feature ID.
   */
  FCesiumMetadataFeatureTable(
      const CesiumGltf::Model& Model,
      const CesiumGltf::Accessor& Accessor,
      const CesiumGltf::FeatureTable& FeatureTable);

private:
  FeatureIDAccessorType _featureIDAccessor;
  TMap<FString, FCesiumMetadataProperty> _properties;

  friend class UCesiumMetadataFeatureTableBlueprintLibrary;
};

class UE_DEPRECATED(
    4.26,
    "UCesiumMetadataFeatureTableBlueprintLibrary is deprecated, use UCesiumFeatureTableBlueprintLibrary instead.")
    UCesiumMetadataFeatureTableBlueprintLibrary;

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataFeatureTableBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  PRAGMA_DISABLE_DEPRECATION_WARNINGS

  /**
   * Gets the number of features in the feature table.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureTable",
      Meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "UCesiumMetadataFeatureTableBlueprintLibrary is deprecated, use UCesiumFeatureTableBlueprintLibrary and FCesiumFeatureTable instead."))
  static int64
  GetNumberOfFeatures(UPARAM(ref)
                          const FCesiumMetadataFeatureTable& FeatureTable);

  /**
   * Gets the feature ID associated with a given vertex.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureTable",
      Meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "UCesiumMetadataFeatureTableBlueprintLibrary is deprecated, use UCesiumFeatureTableBlueprintLibrary and FCesiumFeatureTable instead."))
  static int64 GetFeatureIDForVertex(
      UPARAM(ref) const FCesiumMetadataFeatureTable& FeatureTable,
      int64 VertexIndex);

  /**
   * Gets a map of property name to property value for a given feature.
   *
   * @param featureID The ID of the feature.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureTable",
      Meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "UCesiumMetadataFeatureTableBlueprintLibrary is deprecated, use UCesiumFeatureTableBlueprintLibrary and FCesiumFeatureTable instead."))
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
      Category = "Cesium|Metadata|FeatureTable",
      Meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "UCesiumMetadataFeatureTableBlueprintLibrary is deprecated, use UCesiumFeatureTableBlueprintLibrary and FCesiumFeatureTable instead."))
  static TMap<FString, FString> GetMetadataValuesAsStringForFeatureID(
      UPARAM(ref) const FCesiumMetadataFeatureTable& featureTable,
      int64 FeatureID);

  /**
   * Gets all the properties of the feature table.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureTable",
      Meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "UCesiumMetadataFeatureTableBlueprintLibrary is deprecated, use UCesiumFeatureTableBlueprintLibrary and FCesiumFeatureTable instead."))
  static const TMap<FString, FCesiumMetadataProperty>&
  GetProperties(UPARAM(ref) const FCesiumMetadataFeatureTable& FeatureTable);

  PRAGMA_ENABLE_DEPRECATION_WARNINGS
};
