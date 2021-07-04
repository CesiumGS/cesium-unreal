#pragma once

#include "CesiumMetadata.h"
#include "CesiumMetadataBlueprintLibrary.generated.h"

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataArrayBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static ECesiumMetadataValueType
  GetComponentType(UPARAM(ref) const FCesiumMetadataArray& array);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static bool
  GetBoolean(UPARAM(ref) const FCesiumMetadataArray& array, int64 index);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static int64
  GetInt64(UPARAM(ref) const FCesiumMetadataArray& array, int64 index);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static float
  GetUint64AsFloat(UPARAM(ref) const FCesiumMetadataArray& array, int64 index);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static float
  GetFloat(UPARAM(ref) const FCesiumMetadataArray& array, int64 index);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static float
  GetDoubleAsFloat(UPARAM(ref) const FCesiumMetadataArray& array, int64 index);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static FString
  GetString(UPARAM(ref) const FCesiumMetadataArray& array, int64 index);
};

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataGenericValueBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static ECesiumMetadataValueType
  GetType(UPARAM(ref) const FCesiumMetadataGenericValue& value);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static int64 GetInt64(UPARAM(ref) const FCesiumMetadataGenericValue& value);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static float GetUint64AsFloat(UPARAM(ref)
                                    const FCesiumMetadataGenericValue& value);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static float GetFloat(UPARAM(ref) const FCesiumMetadataGenericValue& value);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static float GetDoubleAsFloat(UPARAM(ref)
                                    const FCesiumMetadataGenericValue& value);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static bool GetBoolean(UPARAM(ref) const FCesiumMetadataGenericValue& value);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static FString GetString(UPARAM(ref)
                               const FCesiumMetadataGenericValue& value);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static FCesiumMetadataArray
  GetArray(UPARAM(ref) const FCesiumMetadataGenericValue& value);
};

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataPropertyBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static ECesiumMetadataValueType
  GetType(UPARAM(ref) const FCesiumMetadataProperty& property);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static int64 GetNumOfFeature(UPARAM(ref) const FCesiumMetadataProperty& property);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static bool GetBoolean(
      UPARAM(ref) const FCesiumMetadataProperty& property,
      int64 featureID);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static int64 GetInt64(
      UPARAM(ref) const FCesiumMetadataProperty& property,
      int64 featureID);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static float GetUint64AsFloat(
      UPARAM(ref) const FCesiumMetadataProperty& property,
      int64 featureID);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static float GetFloat(
      UPARAM(ref) const FCesiumMetadataProperty& property,
      int64 featureID);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static float GetDoubleAsFloat(
      UPARAM(ref) const FCesiumMetadataProperty& property,
      int64 featureID);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static FString GetString(
      UPARAM(ref) const FCesiumMetadataProperty& property,
      int64 featureID);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static FCesiumMetadataArray GetArray(
      UPARAM(ref) const FCesiumMetadataProperty& property,
      int64 featureID);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static FCesiumMetadataGenericValue GetGenericValue(
      UPARAM(ref) const FCesiumMetadataProperty& property,
      int64 featureID);
};

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataFeatureTableBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static int64 GetNumOfFeature(UPARAM(ref) const FCesiumMetadataFeatureTable& featureTable);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static TMap<FString, FCesiumMetadataGenericValue> GetValuesForFeatureID(
      UPARAM(ref) const FCesiumMetadataFeatureTable& featureTable,
      int64 featureID);

  TMap<FString, FString> static GetValuesAsStringsForFeatureID(
      UPARAM(ref) const FCesiumMetadataFeatureTable& featureTable,
      int64 featureID);

  static const TMap<FString, FCesiumMetadataProperty>&
  GetProperties(UPARAM(ref) const FCesiumMetadataFeatureTable& featureTable);
};

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataPrimitiveBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static const TArray<FCesiumMetadataFeatureTable>&
  GetFeatureTables(UPARAM(ref)
                       const FCesiumMetadataPrimitive& metadataPrimitive);
};

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataUtilityBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static FCesiumMetadataPrimitive
  GetPrimitiveMetadata(const UPrimitiveComponent* component);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static TMap<FString, FCesiumMetadataGenericValue>
  GetMetadataValuesForFace(const UPrimitiveComponent* component, int64 faceID);

  UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cesium|Metadata")
  static TMap<FString, FString>
  GetMetadataValuesAsStringForFace(const UPrimitiveComponent* component, int64 faceID);
};

