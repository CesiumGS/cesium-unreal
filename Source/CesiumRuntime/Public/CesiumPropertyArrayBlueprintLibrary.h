// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumPropertyArray.h"
#include "CesiumMetadataValue.h"
#include "CesiumPropertyArrayBlueprintLibrary.generated.h"

/**
 * Blueprint library functions for acting on an array property in EXT_structural_metadata.
 */
UCLASS()
class CESIUMRUNTIME_API UCesiumPropertyArrayBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Gets the best-fitting Blueprints type for the elements of this array.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyArray")
  static ECesiumMetadataBlueprintType
  GetElementBlueprintType(UPARAM(ref) const FCesiumPropertyArray& array);

  /**
   * Gets the true value type of the elements in the array. Many of these types
   * are not accessible from Blueprints, but can be converted to a
   * Blueprint-accessible type.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyArray")
  static FCesiumMetadataValueType
  GetElementValueType(UPARAM(ref) const FCesiumPropertyArray& array);

  /**
   * Gets the number of elements in the array. Returns 0 if the elements have
   * an unknown type.
   *
   * @return The number of elements in the array.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyArray")
  static int64 GetSize(UPARAM(ref) const FCesiumPropertyArray& Array);

  /**
   * Retrieves an element from the array as a FCesiumMetadataValue. The value
   * can then be retrieved as another Blueprints type.
   *
   * If the index is out-of-bounds, this returns a bogus FCesiumMetadataValue of an unknown type.
   *
   * @param Index The index of the array element to retrieve.
   * @return The element as a FCesiumMetadataValue.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyArray")
  static FCesiumMetadataValue
  GetValue(UPARAM(ref) const FCesiumPropertyArray& Array, int64 Index);
};
