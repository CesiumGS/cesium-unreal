// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/MetadataArrayView.h"
#include "CesiumMetadataValueType.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ObjectMacros.h"
#include <string_view>
#include <variant>
#include "CesiumMetadataArray.generated.h"

/**
 * A Blueprint-accessible wrapper for an array property in glTF metadata.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumMetadataArray {
  GENERATED_USTRUCT_BODY()

private:
  using ArrayType = std::variant<
      CesiumGltf::MetadataArrayView<int8_t>,
      CesiumGltf::MetadataArrayView<uint8_t>,
      CesiumGltf::MetadataArrayView<int16_t>,
      CesiumGltf::MetadataArrayView<uint16_t>,
      CesiumGltf::MetadataArrayView<int32_t>,
      CesiumGltf::MetadataArrayView<uint32_t>,
      CesiumGltf::MetadataArrayView<int64_t>,
      CesiumGltf::MetadataArrayView<uint64_t>,
      CesiumGltf::MetadataArrayView<float>,
      CesiumGltf::MetadataArrayView<double>,
      CesiumGltf::MetadataArrayView<bool>,
      CesiumGltf::MetadataArrayView<std::string_view>>;

public:
  /**
   * Construct an empty array with unknown type.
   */
  FCesiumMetadataArray() : _value(), _type(ECesiumMetadataValueType::None) {}

  /**
   * Construct a non-empty array.
   * @param value The metadata array view that will be stored in this struct
   */
  template <typename T>
  FCesiumMetadataArray(CesiumGltf::MetadataArrayView<T> value)
      : _value(value), _type(ECesiumMetadataValueType::None) {
    if (holdsArrayAlternative<bool>(_value)) {
      _type = ECesiumMetadataValueType::Boolean;
    } else if (holdsArrayAlternative<uint8_t>(_value)) {
      _type = ECesiumMetadataValueType::Byte;
    } else if (
        holdsArrayAlternative<int8_t>(_value) ||
        holdsArrayAlternative<int16_t>(_value) ||
        holdsArrayAlternative<uint16_t>(_value) ||
        holdsArrayAlternative<int32_t>(_value)) {
      _type = ECesiumMetadataValueType::Integer;
    } else if (
        holdsArrayAlternative<uint32_t>(_value) ||
        holdsArrayAlternative<int64_t>(_value)) {
      _type = ECesiumMetadataValueType::Integer64;
    } else if (holdsArrayAlternative<uint64_t>(_value)) {
      _type = ECesiumMetadataValueType::String;
    } else if (holdsArrayAlternative<float>(_value)) {
      _type = ECesiumMetadataValueType::Float;
    } else if (holdsArrayAlternative<double>(_value)) {
      _type = ECesiumMetadataValueType::String;
    } else {
      _type = ECesiumMetadataValueType::String;
    }
  }

  /**
   * Queries the type of the elements in the array. For the most precise
   * representation of the property's values, you should retrieve them using
   * this type.
   *
   * @return The component type of the array.
   */
  ECesiumMetadataValueType GetComponentType() const;

  /**
   * Queries the number of elements in the array.
   * This method returns 0 if component type is ECesiumMetadataValueType::None
   *
   * @return The number of elements in the array.
   */
  size_t GetSize() const;

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
   * @param index The index of the array element to retrieve.
   * @param defaultValue The default value to use if the index is invalid
   * or the element's value cannot be converted.
   * @return The element value.
   */
  bool GetBoolean(int64 index, bool defaultValue) const;

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
   * @param index The index of the array element to retrieve.
   * @param defaultValue The default value to use if the index is invalid
   * or the element's value cannot be converted.
   * @return The element value.
   */
  uint8 GetByte(int64 index, uint8 defaultValue) const;

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
   * @param index The index of the array element to retrieve.
   * @param defaultValue The default value to use if the index is invalid
   * or the element's value cannot be converted.
   * @return The element value.
   */
  int32 GetInteger(int64 index, int32 defaultValue) const;

  /**
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
   * @param index The index of the array element to retrieve.
   * @param defaultValue The default value to use if the index is invalid
   * or the element's value cannot be converted.
   * @return The element value.
   */
  int64 GetInteger64(int64 index, int64 defaultValue) const;

  /**
   * Retrieves an element from the array and attempts to convert it to a 32-bit
   * floating-point value.
   *
   * If the element is a singe-precision floating-point number, is is returned.
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
   * @param index The index of the array element to retrieve.
   * @param defaultValue The default value to use if the index is invalid
   * or the element's value cannot be converted.
   * @return The element value.
   */
  float GetFloat(int64 index, float defaultValue) const;

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
   * @param index The index of the array element to retrieve.
   * @param defaultValue The default value to use if the index is invalid
   * or the element's value cannot be converted.
   * @return The element value.
   */
  FString GetString(int64 index, const FString& defaultValue) const;

private:
  template <typename T, typename... VariantType>
  static bool
  holdsArrayAlternative(const std::variant<VariantType...>& variant) {
    return std::holds_alternative<CesiumGltf::MetadataArrayView<T>>(variant);
  }

  ArrayType _value;
  ECesiumMetadataValueType _type;
};

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
   * Queries the number of elements in the array.
   * This method returns 0 if component type is ECesiumMetadataValueType::None
   *
   * @return The number of elements in the array.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Array")
  static int64 GetSize(UPARAM(ref) const FCesiumMetadataArray& Array);

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
   * @param Index The index of the array element to retrieve.
   * @param DefaultValue The default value to use if the index is invalid
   * or the element's value cannot be converted.
   * @return The element value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Array")
  static bool GetBoolean(
      UPARAM(ref) const FCesiumMetadataArray& Array,
      int64 Index,
      bool DefaultValue);

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
   * @param Index The index of the array element to retrieve.
   * @param DefaultValue The default value to use if the index is invalid
   * or the element's value cannot be converted.
   * @return The element value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Array")
  static uint8 GetByte(
      UPARAM(ref) const FCesiumMetadataArray& Array,
      int64 Index,
      uint8 DefaultValue);

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
   * @param Index The index of the array element to retrieve.
   * @param DefaultValue The default value to use if the index is invalid
   * or the element's value cannot be converted.
   * @return The element value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Array")
  static int32 GetInteger(
      UPARAM(ref) const FCesiumMetadataArray& Array,
      int64 Index,
      int32 DefaultValue);

  /**
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
   * @param index The index of the array element to retrieve.
   * @param defaultValue The default value to use if the index is invalid
   * or the element's value cannot be converted.
   * @return The element value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Array")
  static int64 GetInteger64(
      UPARAM(ref) const FCesiumMetadataArray& Array,
      int64 Index,
      int64 DefaultValue);

  /**
   * Retrieves an element from the array and attempts to convert it to a 32-bit
   * floating-point value.
   *
   * If the element is a singe-precision floating-point number, is is returned.
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
   * @param Index The index of the array element to retrieve.
   * @param DefaultValue The default value to use if the index is invalid
   * or the element's value cannot be converted.
   * @return The element value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Array")
  static float GetFloat(
      UPARAM(ref) const FCesiumMetadataArray& array,
      int64 index,
      float DefaultValue);

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
   * @param Index The index of the array element to retrieve.
   * @param DefaultValue The default value to use if the index is invalid
   * or the element's value cannot be converted.
   * @return The element value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Array")
  static FString GetString(
      UPARAM(ref) const FCesiumMetadataArray& Array,
      int64 Index,
      const FString& DefaultValue);
};
