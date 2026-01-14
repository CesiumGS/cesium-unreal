// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumMetadataValue.h"
#include "CesiumPropertyArray.h"
#include "CesiumPropertyArrayBlueprintLibrary.generated.h"

/**
 * Blueprint library functions for acting on an array value from a property in
 * EXT_structural_metadata.
 */
UCLASS()
class CESIUMRUNTIME_API UCesiumPropertyArrayBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Gets the best-fitting Blueprints type for the elements of this array.
   *
   * @param array The array.
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
   *
   * @param array The array.
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
   * @param Array The array.
   * @return The number of elements in the array.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyArray")
  static int64 GetArraySize(UPARAM(ref) const FCesiumPropertyArray& Array);

  /**
   * Retrieves an element from the array as a FCesiumMetadataValue. The value
   * can then be retrieved as a specific Blueprints type.
   *
   * If the index is out-of-bounds, this returns a bogus FCesiumMetadataValue of
   * an unknown type.
   *
   * @param Array The array.
   * @param Index The index of the array element to retrieve.
   * @return The element as a FCesiumMetadataValue.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyArray")
  static FCesiumMetadataValue
  GetValue(UPARAM(ref) const FCesiumPropertyArray& Array, int64 Index);

  /**
   * Prints the contents of the array to a human-readable string in the format
   * "[A, B, C, ... Z]".
   * @param Array The array.
   * @return A string capturing the contents of the array.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyArray")
  static FString ToString(
      UPARAM(ref) const FCesiumPropertyArray& Array);

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  /**
   * Gets the best-fitting Blueprints type for the elements of this array.
   *
   * @param array The array.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Meta =
          (DeprecatedFunction,
           DeprecationMessage = "Use GetElementBlueprintType instead."))
  static ECesiumMetadataBlueprintType
  GetBlueprintComponentType(UPARAM(ref) const FCesiumPropertyArray& array);

  /**
   * Gets true type of the elements in the array. Many of these types are not
   * accessible from Blueprints, but can be converted to a Blueprint-accessible
   * type.
   *
   * @param array The array.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "CesiumMetadataTrueType is deprecated. Use GetElementValueType instead."))
  static ECesiumMetadataTrueType_DEPRECATED
  GetTrueComponentType(UPARAM(ref) const FCesiumPropertyArray& array);

  /**
   * Gets the number of elements in the array. Returns 0 if the elements have
   * an unknown type.
   *
   * @param Array The array.
   * @return The number of elements in the array.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Meta =
          (DeprecatedFunction,
           DeprecationMessage = "Use GetArraySize instead."))
  static int64 GetSize(UPARAM(ref) const FCesiumPropertyArray& Array);

  /**
   * Retrieves an element from the array and attempts to convert it to a Boolean
   * value.
   *
   * If the element is boolean, it is returned directly.
   *
   * If the element is numeric, zero is converted to false, while any other
   * value is converted to true.
   *
   * If the element is a string, "0", "false", and "no" (case-insensitive) are
   * converted to false, while "1", "true", and "yes" are converted to true.
   * All other strings, including strings that can be converted to numbers,
   * will return the default value.
   *
   * Other types of elements will return the default value.
   *
   * @param Array The array.
   * @param Index The index of the array element to retrieve.
   * @param DefaultValue The default value to use if the index is invalid
   * or the element's value cannot be converted.
   * @return The element value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "GetBoolean is deprecated for metadata arrays. Use GetValue instead."))
  static bool GetBoolean(
      UPARAM(ref) const FCesiumPropertyArray& Array,
      int64 Index,
      bool DefaultValue = false);

  /**
   * Retrieves an element from the array and attempts to convert it to an
   * unsigned 8-bit integer value.
   *
   * If the element is an integer and between 0 and 255, it is returned
   * directly.
   *
   * If the element is a floating-point number, it is truncated (rounded
   * toward zero).
   *
   * If the element is a boolean, 0 is returned for false and 1 for true.
   *
   * If the element is a string and the entire string can be parsed as an
   * integer between 0 and 255, the parsed value is returned. The string is
   * parsed in a locale-independent way and does not support use of a comma or
   * other character to group digits.
   *
   * Otherwise, the default value is returned.
   *
   * @param Array The array.
   * @param Index The index of the array element to retrieve.
   * @param DefaultValue The default value to use if the index is invalid
   * or the element's value cannot be converted.
   * @return The element value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "GetByte is deprecated on arrays. Use GetValue instead."))
  static uint8 GetByte(
      UPARAM(ref) const FCesiumPropertyArray& Array,
      int64 Index,
      uint8 DefaultValue = 0);

  /**
   * Retrieves an element from the array and attempts to convert it to a signed
   * 32-bit integer value.
   *
   * If the element is an integer and between -2,147,483,647 and 2,147,483,647,
   * it is returned directly.
   *
   * If the element is a floating-point number, it is truncated (rounded
   * toward zero).
   *
   * If the element is a boolean, 0 is returned for false and 1 for true.
   *
   * If the element is a string and the entire string can be parsed as an
   * integer in the valid range, the parsed value is returned. If it can be
   * parsed as a floating-point number, the parsed value is truncated (rounded
   * toward zero). In either case, the string is parsed in a locale-independent
   * way and does not support use of a comma or other character to group digits.
   *
   * Otherwise, the default value is returned.
   *
   * @param Array The array.
   * @param Index The index of the array element to retrieve.
   * @param DefaultValue The default value to use if the index is invalid
   * or the element's value cannot be converted.
   * @return The element value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "GetInteger is deprecated for metadata arrays. Use GetValue instead."))
  static int32 GetInteger(
      UPARAM(ref) const FCesiumPropertyArray& Array,
      int64 Index,
      int32 DefaultValue = 0);

  /**
   * This function is deprecated. Use Cesium > Metadata > Property Array >
   * GetValue instead.
   *
   * Retrieves an element from the array and attempts to convert it to a signed
   * 64-bit integer value.
   *
   * If the element is an integer and between -2^63-1 and 2^63-1, it is returned
   * directly.
   *
   * If the element is a floating-point number, it is truncated (rounded
   * toward zero).
   *
   * If the element is a boolean, 0 is returned for false and 1 for true.
   *
   * If the element is a string and the entire string can be parsed as an
   * integer in the valid range, the parsed value is returned. If it can be
   * parsed as a floating-point number, the parsed value is truncated (rounded
   * toward zero). In either case, the string is parsed in a locale-independent
   * way and does not support use of a comma or other character to group digits.
   *
   * Otherwise, the default value is returned.
   *
   * @param Array The array.
   * @param Index The index of the array element to retrieve.
   * @param DefaultValue The default value to use if the index is invalid
   * or the element's value cannot be converted.
   * @return The element value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "GetInteger64 is deprecated for metadata arrays. Use GetValue instead."))
  static int64 GetInteger64(
      UPARAM(ref) const FCesiumPropertyArray& Array,
      int64 Index,
      int64 DefaultValue = 0);

  /**
   * Retrieves an element from the array and attempts to convert it to a 32-bit
   * floating-point value.
   *
   * If the element is a single-precision floating-point number, is is returned.
   *
   * If the element is an integer or double-precision floating-point number,
   * it is converted to the closest representable single-precision
   * floating-point number.
   *
   * If the element is a boolean, 0.0 is returned for false and 1.0 for true.
   *
   * If the element is a string and the entire string can be parsed as a
   * number, the parsed value is returned. The string is parsed in a
   * locale-independent way and does not support use of a comma or other
   * character to group digits.
   *
   * Otherwise, the default value is returned.
   *
   * @param array The array.
   * @param index The index of the array element to retrieve.
   * @param DefaultValue The default value to use if the index is invalid
   * or the element's value cannot be converted.
   * @return The element value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "GetFloat is deprecated for metadata arrays. Use GetValue instead."))
  static float GetFloat(
      UPARAM(ref) const FCesiumPropertyArray& array,
      int64 index,
      float DefaultValue = 0.0f);

  /**
   * Retrieves an element from the array and attempts to convert it to a 64-bit
   * floating-point value.
   *
   * If the element is a single- or double-precision floating-point number, is
   * is returned.
   *
   * If the element is an integer, it is converted to the closest representable
   * double-precision floating-point number.
   *
   * If the element is a boolean, 0.0 is returned for false and 1.0 for true.
   *
   * If the element is a string and the entire string can be parsed as a
   * number, the parsed value is returned. The string is parsed in a
   * locale-independent way and does not support use of a comma or other
   * character to group digits.
   *
   * Otherwise, the default value is returned.
   *
   * @param array The array.
   * @param index The index of the array element to retrieve.
   * @param DefaultValue The default value to use if the index is invalid
   * or the element's value cannot be converted.
   * @return The element value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "GetFloat64 is deprecated for metadata arrays. Use GetValue instead."))
  static double GetFloat64(
      UPARAM(ref) const FCesiumPropertyArray& array,
      int64 index,
      double DefaultValue);

  /**
   * Retrieves an element from the array and attempts to convert it to a string
   * value.
   *
   * Numeric elements are converted to a string with `FString::Format`, which
   * uses the current locale.
   *
   * Boolean elements are converted to "true" or "false".
   *
   * String elements are returned directly.
   *
   * @param Array The array.
   * @param Index The index of the array element to retrieve.
   * @param DefaultValue The default value to use if the index is invalid
   * or the element's value cannot be converted.
   * @return The element value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "GetString is deprecated for metadata arrays. Use GetValue instead."))
  static FString GetString(
      UPARAM(ref) const FCesiumPropertyArray& Array,
      int64 Index,
      const FString& DefaultValue = "");

  PRAGMA_ENABLE_DEPRECATION_WARNINGS
};
