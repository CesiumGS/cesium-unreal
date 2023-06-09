// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumMetadataValue.h"
#include "CesiumPropertyTableProperty.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ObjectMacros.h"
#include "CesiumPropertyTable.generated.h"

namespace CesiumGltf {
struct Model;
struct ExtensionExtStructuralMetadataPropertyTable;
} // namespace CesiumGltf

UENUM(BlueprintType)
enum class ECesiumPropertyTableStatus : uint8 {
  Valid = 0,
  ErrorInvalidMetadataExtension,
  ErrorInvalidPropertyTableClass
};

/**
 * A Blueprint-accessible wrapper for a glTF property table. A property table is
 * a collection of properties for the features in a mesh. It knows how to
 * look up the metadata values associated with a given feature ID.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumPropertyTable {
  GENERATED_USTRUCT_BODY()

public:
  /**
   * Construct an empty property table instance.
   */
  FCesiumPropertyTable()
      : _status(ECesiumPropertyTableStatus::ErrorInvalidMetadataExtension){};

  /**
   * Constructs a property table from a glTF Property Table.
   *
   * @param Model The model that stores EXT_structural_metadata.
   * @param PropertyTable The target property table.
   */
  FCesiumPropertyTable(
      const CesiumGltf::Model& Model,
      const CesiumGltf::ExtensionExtStructuralMetadataPropertyTable&
          PropertyTable);

private:
  ECesiumPropertyTableStatus _status;
  int64 _count;
  TMap<FString, FCesiumPropertyTableProperty> _properties;

  friend class UCesiumPropertyTableBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumPropertyTableBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Gets the status of the property table. If an error occurred while parsing
   * the property table from the glTF extension, this briefly conveys why.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTable")
  static ECesiumPropertyTableStatus
  GetPropertyTableStatus(UPARAM(ref) const FCesiumPropertyTable& PropertyTable);

  /**
   * Gets the number of elements in the property table. In other words, this is
   * how many values each property in the table is expected to have. If an error
   * occurred while parsing the property table, this returns zero.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTable")
  static int64
  GetNumberOfElements(UPARAM(ref) const FCesiumPropertyTable& PropertyTable);

  /**
   * Gets all the properties of the property table, mapped by property name.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTable")
  static const TMap<FString, FCesiumPropertyTableProperty>&
  GetProperties(UPARAM(ref) const FCesiumPropertyTable& PropertyTable);

  /**
   * Gets all of the property values for a given feature, mapped by property
   * name.
   *
   * @param featureID The ID of the feature.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTable")
  static TMap<FString, FCesiumMetadataValue> GetMetadataValuesForFeatureID(
      UPARAM(ref) const FCesiumPropertyTable& PropertyTable,
      int64 FeatureID);

  /**
   * Gets all of the property values for a given feature as strings, mapped by
   * property name.
   *
   * @param FeatureID The ID of the feature.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTable")
  static TMap<FString, FString> GetMetadataValuesAsStringsForFeatureID(
      UPARAM(ref) const FCesiumPropertyTable& PropertyTable,
      int64 FeatureID);
};
