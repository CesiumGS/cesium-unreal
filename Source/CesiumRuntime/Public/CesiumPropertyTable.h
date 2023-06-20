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
  FString _name;
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
   * Gets the name of the property table. If no name was specified in the glTF
   * extension, this returns an empty string.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTable")
  static const FString&
  GetPropertyTableName(UPARAM(ref) const FCesiumPropertyTable& PropertyTable);

  /**
   * Gets the size of the property table. In other words, this is
   * how many values each property in the table is expected to have. If an error
   * occurred while parsing the property table, this returns zero.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTable")
  static int64
  GetPropertyTableSize(UPARAM(ref) const FCesiumPropertyTable& PropertyTable);

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
   * Gets the names of the properties in this property table.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTable")
  static const TArray<FString>
  GetPropertyNames(UPARAM(ref) const FCesiumPropertyTable& PropertyTable);

  /**
   * Retrieve a FCesiumPropertyTableProperty by name. If the property table
   * does not contain a property with that name, this returns an invalid
   * FCesiumPropertyTableProperty.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTable")
  static const FCesiumPropertyTableProperty& FindProperty(
      UPARAM(ref) const FCesiumPropertyTable& PropertyTable,
      const FString& PropertyName);

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
