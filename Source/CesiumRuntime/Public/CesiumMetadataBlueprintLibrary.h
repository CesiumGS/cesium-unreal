#pragma once

#include "CesiumMetadata.h"
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CesiumMetadataBlueprintLibrary.generated.h"

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataArrayBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Query the component type of the array value.
   * This method should be used first before retrieving the data of the array.
   * If the data that is tried to retrieve is different from the stored data
   * type, the current program will be aborted.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Array")
  static ECesiumMetadataValueType
  GetComponentType(UPARAM(ref) const FCesiumMetadataArray& array);

  /**
   * Query the number of elements in the array.
   * This method returns 0 if component type is ECesiumMetadataValueType::None
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Array")
  static int64 GetSize(UPARAM(ref) const FCesiumMetadataArray& array);

  /**
   * Retrieve the component at index i as an int64_t value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Array")
  static int64
  GetInt64(UPARAM(ref) const FCesiumMetadataArray& array, int64 index);

  /**
   * Retrieve the component at index i as an uint64_t value. However,
   * because blueprint cannot work with uint64, the type will be converted
   * to float, which will incur the loss of precision
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Array")
  static float
  GetUint64AsFloat(UPARAM(ref) const FCesiumMetadataArray& array, int64 index);

  /**
   * Retrieve the component at index i as a float value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Array")
  static float
  GetFloat(UPARAM(ref) const FCesiumMetadataArray& array, int64 index);

  /**
   * Retrieve the component at index i as a double value. However,
   * because blueprint cannot work with double, the type will be converted
   * to float, which will incur the loss of precision
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Array")
  static float
  GetDoubleAsFloat(UPARAM(ref) const FCesiumMetadataArray& array, int64 index);

  /**
   * Retrieve the component at index i as a boolean value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Array")
  static bool
  GetBoolean(UPARAM(ref) const FCesiumMetadataArray& array, int64 index);

  /**
   * Retrieve the component at index i as a string value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Array")
  static FString
  GetString(UPARAM(ref) const FCesiumMetadataArray& array, int64 index);
};

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataGenericValueBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Query the type of the value.
   * This method should be used first before retrieving the stored value.
   * If the data that is tried to retrieve is different from the stored data
   * type, the current program will be aborted.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|GenericValue")
  static ECesiumMetadataValueType
  GetType(UPARAM(ref) const FCesiumMetadataGenericValue& value);

  /**
   * Retrieve the value as an int64_t.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|GenericValue")
  static int64 GetInt64(UPARAM(ref) const FCesiumMetadataGenericValue& value);

  /**
   * Retrieve the value as an uint64_t value. However,
   * because blueprint cannot work with uint64, the type will be converted
   * to float, which will incur the loss of precision
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|GenericValue")
  static float GetUint64AsFloat(UPARAM(ref)
                                    const FCesiumMetadataGenericValue& value);

  /**
   * Retrieve the value as a float value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|GenericValue")
  static float GetFloat(UPARAM(ref) const FCesiumMetadataGenericValue& value);

  /**
   * Retrieve the value as a double value. However,
   * because blueprint cannot work with double, the type will be converted
   * to float, which will incur the loss of precision
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|GenericValue")
  static float GetDoubleAsFloat(UPARAM(ref)
                                    const FCesiumMetadataGenericValue& value);

  /**
   * Retrieve the value as a boolean value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|GenericValue")
  static bool GetBoolean(UPARAM(ref) const FCesiumMetadataGenericValue& value);

  /**
   * Retrieve the value as a string value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|GenericValue")
  static FString GetString(UPARAM(ref)
                               const FCesiumMetadataGenericValue& value);

  /**
   * Retrieve the value as a generic array value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|GenericValue")
  static FCesiumMetadataArray
  GetArray(UPARAM(ref) const FCesiumMetadataGenericValue& value);

  /**
   * Convert the stored value to string for display purpose.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|GenericValue")
  static FString ToString(UPARAM(ref) const FCesiumMetadataGenericValue& value);
};

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataPropertyBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Query the type of the property.
   * This method should be used first before retrieving the stored value.
   * If the data that is tried to retrieve is different from the stored data
   * type, the current program will be aborted.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Property")
  static ECesiumMetadataValueType
  GetType(UPARAM(ref) const FCesiumMetadataProperty& property);

  /**
   * Query the number of features in the property.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Property")
  static int64 GetNumOfFeatures(UPARAM(ref)
                                    const FCesiumMetadataProperty& property);

  /**
   * Retrieve the feature value at index featureID as a boolean value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Property")
  static bool GetBoolean(
      UPARAM(ref) const FCesiumMetadataProperty& property,
      int64 featureID);

  /**
   * Retrieve the feature value at index featureID as an int64_t value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Property")
  static int64 GetInt64(
      UPARAM(ref) const FCesiumMetadataProperty& property,
      int64 featureID);

  /**
   * Retrieve the feature value at index featureID as an uint64_t value.
   * However, because blueprint cannot work with uint64, the type will be
   * converted to float, which will incur the loss of precision
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Property")
  static float GetUint64AsFloat(
      UPARAM(ref) const FCesiumMetadataProperty& property,
      int64 featureID);

  /**
   * Retrieve the feature value at index featureID as a float value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Property")
  static float GetFloat(
      UPARAM(ref) const FCesiumMetadataProperty& property,
      int64 featureID);

  /**
   * Retrieve the feature value at index featureID as a double value. However,
   * because blueprint cannot work with double, the type will be converted
   * to float, which will incur the loss of precision
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Property")
  static float GetDoubleAsFloat(
      UPARAM(ref) const FCesiumMetadataProperty& property,
      int64 featureID);

  /**
   * Retrieve the feature value at index featureID as a string value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Property")
  static FString GetString(
      UPARAM(ref) const FCesiumMetadataProperty& property,
      int64 featureID);

  /**
   * Retrieve the feature value at index featureID as an array value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Property")
  static FCesiumMetadataArray GetArray(
      UPARAM(ref) const FCesiumMetadataProperty& property,
      int64 featureID);

  /**
   * Convert the underlying value to a generic value. Convenient for storing
   * the value in the container like TArray or TMap
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Property")
  static FCesiumMetadataGenericValue GetGenericValue(
      UPARAM(ref) const FCesiumMetadataProperty& property,
      int64 featureID);
};

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataFeatureTableBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Query the number of features in the feature table.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureTable")
  static int64
  GetNumOfFeatures(UPARAM(ref) const FCesiumMetadataFeatureTable& featureTable);

  /**
   * Query the feature ID based on a vertex.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureTable")
  static int64 GetFeatureIDForVertex(
      UPARAM(ref) const FCesiumMetadataFeatureTable& featureTable,
      int64 vertexIdx);

  /**
   * Return the map of a feature that maps feature's property name to value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|FeatureTable")
  static TMap<FString, FCesiumMetadataGenericValue> GetValuesForFeatureID(
      UPARAM(ref) const FCesiumMetadataFeatureTable& featureTable,
      int64 featureID);

  /**
   * Return the map of a feature that maps feature's property name to value as
   * string.
   */
  TMap<FString, FString> static GetValuesAsStringsForFeatureID(
      UPARAM(ref) const FCesiumMetadataFeatureTable& featureTable,
      int64 featureID);

  /**
   * Get all the properties of a feature table.
   */
  static const TMap<FString, FCesiumMetadataProperty>&
  GetProperties(UPARAM(ref) const FCesiumMetadataFeatureTable& featureTable);
};

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataPrimitiveBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Get all the feature tables that are associated with the primitive.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Property")
  static const TArray<FCesiumMetadataFeatureTable>&
  GetFeatureTables(UPARAM(ref)
                       const FCesiumMetadataPrimitive& metadataPrimitive);
};

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataUtilityBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Get the primitive metadata of a Gltf primitive component. If component is
   * not a Cesium Gltf primitive component, the returned metadata is empty
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Utility")
  static FCesiumMetadataPrimitive
  GetPrimitiveMetadata(const UPrimitiveComponent* component);

  /**
   * Get the metadata of a face of a gltf primitive component. If the component
   * is not a Cesium Gltf primitive component, the returned metadata is empty
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Utility")
  static TMap<FString, FCesiumMetadataGenericValue>
  GetMetadataValuesForFace(const UPrimitiveComponent* component, int64 faceID);

  /**
   * Get the metadata as string of a face of a gltf primitive component. If the
   * component is not a Cesium Gltf primitive component, the returned metadata
   * is empty
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Utility")
  static TMap<FString, FString> GetMetadataValuesAsStringForFace(
      const UPrimitiveComponent* component,
      int64 faceID);
};
