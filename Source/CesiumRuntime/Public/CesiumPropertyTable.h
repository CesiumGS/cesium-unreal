// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumMetadataEnum.h"
#include "CesiumMetadataValue.h"
#include "CesiumPropertyTableProperty.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ObjectMacros.h"
#include "CesiumPropertyTable.generated.h"

namespace CesiumGltf {
struct Model;
struct PropertyTable;
} // namespace CesiumGltf

/**
 * @brief Reports the status of a FCesiumPropertyTable. If the property table
 * cannot be accessed, this briefly indicates why.
 */
UENUM(BlueprintType)
enum class ECesiumPropertyTableStatus : uint8 {
  /* The property table is valid. */
  Valid = 0,
  /* The property table instance was not initialized from an actual glTF
     property table. */
  ErrorInvalidPropertyTable,
  /* The property table's class could be found in the schema of the metadata
     extension. */
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
      : _status(ECesiumPropertyTableStatus::ErrorInvalidPropertyTable){};

  /**
   * Constructs a property table from a glTF Property Table.
   *
   * @param Model The model that stores EXT_structural_metadata.
   * @param PropertyTable The target property table.
   */
  FCesiumPropertyTable(
      const CesiumGltf::Model& Model,
      const CesiumGltf::PropertyTable& PropertyTable)
      : FCesiumPropertyTable(
            Model,
            PropertyTable,
            FCesiumMetadataEnumCollection::GetOrCreateFromModel(Model)) {}

  /**
   * Constructs a property table from a glTF Property Table.
   *
   * @param Model The model that stores EXT_structural_metadata.
   * @param PropertyTable The target property table.
   * @param EnumCollection The enum collection to use, if any.
   */
  FCesiumPropertyTable(
      const CesiumGltf::Model& Model,
      const CesiumGltf::PropertyTable& PropertyTable,
      const TSharedPtr<FCesiumMetadataEnumCollection>& EnumCollection);

  /**
   * Gets the name of the metadata class that this property table conforms to.
   */
  FString getClassName() const { return _className; }

private:
  ECesiumPropertyTableStatus _status;
  FString _name;
  FString _className;

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
   *
   * @param PropertyTable The property table.
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
   *
   * @param PropertyTable The property table.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTable")
  static const FString&
  GetPropertyTableName(UPARAM(ref) const FCesiumPropertyTable& PropertyTable);

  /**
   * Gets the number of values each property in the table is expected to have.
   * If an error occurred while parsing the property table, this returns zero.
   *
   * @param PropertyTable The property table.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTable")
  static int64
  GetPropertyTableCount(UPARAM(ref) const FCesiumPropertyTable& PropertyTable);

  /**
   * Gets all the properties of the property table, mapped by property name.
   *
   * @param PropertyTable The property table.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTable")
  static const TMap<FString, FCesiumPropertyTableProperty>&
  GetProperties(UPARAM(ref) const FCesiumPropertyTable& PropertyTable);

  /**
   * Gets the names of the properties in this property table.
   *
   * @param PropertyTable The property table.
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
   *
   * @param PropertyTable The property table.
   * @param PropertyName The name of the property to find.
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
   * name. This will only include values from valid property table properties.
   *
   * If the feature ID is out-of-bounds, the returned map will be empty.
   *
   * @param PropertyTable The property table.
   * @param FeatureID The ID of the feature.
   * @return The property values mapped by property name.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTable")
  static TMap<FString, FCesiumMetadataValue> GetMetadataValuesForFeature(
      UPARAM(ref) const FCesiumPropertyTable& PropertyTable,
      int64 FeatureID);

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  /**
   * Gets all of the property values for a given feature as strings, mapped by
   * property name. This will only include values from valid property table
   * properties.
   *
   * Array properties cannot be converted to strings, so empty strings
   * will be returned for their values.
   *
   * If the feature ID is out-of-bounds, the returned map will be empty.
   *
   * @param PropertyTable The property table.
   * @param FeatureID The ID of the feature.
   * @return The property values as strings mapped by property name.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTable",
      Meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "Use GetValuesAsStrings to convert the output of GetMetadataValuesForFeature instead."))
  static TMap<FString, FString> GetMetadataValuesForFeatureAsStrings(
      UPARAM(ref) const FCesiumPropertyTable& PropertyTable,
      int64 FeatureID);
  PRAGMA_ENABLE_DEPRECATION_WARNINGS
};
