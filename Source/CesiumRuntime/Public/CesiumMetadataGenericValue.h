// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/MetadataArrayView.h"
#include "CesiumMetadataArray.h"
#include "CesiumMetadataValueType.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ObjectMacros.h"
#include "CesiumMetadataGenericValue.generated.h"

/**
 * Provide a wrapper for scalar and array value.
 * This struct is provided to make it work with blueprint.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumMetadataGenericValue {
  GENERATED_USTRUCT_BODY()

private:
  using ValueType = std::variant<
      std::monostate,
      int8_t,
      uint8_t,
      int16_t,
      uint16_t,
      int32_t,
      uint32_t,
      int64_t,
      uint64_t,
      float,
      double,
      bool,
      std::string_view,
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
   * Construct an empty value with unknown type.
   */
  FCesiumMetadataGenericValue()
      : _value(std::monostate{}), _type(ECesiumMetadataValueType::None) {}

  /**
   * Construct a scalar or array value.
   *
   * @param value The value to be stored in this struct
   */
  template <typename T>
  explicit FCesiumMetadataGenericValue(const T& value)
      : _value(value), _type(ECesiumMetadataValueType::None) {
    if (std::holds_alternative<std::monostate>(_value)) {
      _type = ECesiumMetadataValueType::None;
    } else if (std::holds_alternative<bool>(_value)) {
      _type = ECesiumMetadataValueType::Boolean;
    } else if (std::holds_alternative<uint8_t>(_value)) {
      _type = ECesiumMetadataValueType::Byte;
    } else if (
        std::holds_alternative<int8_t>(_value) ||
        std::holds_alternative<int16_t>(_value) ||
        std::holds_alternative<uint16_t>(_value) ||
        std::holds_alternative<int32_t>(_value)) {
      _type = ECesiumMetadataValueType::Integer;
    } else if (
        std::holds_alternative<uint32_t>(_value) ||
        std::holds_alternative<int64_t>(_value)) {
      _type = ECesiumMetadataValueType::Integer64;
    } else if (std::holds_alternative<uint64_t>(_value)) {
      _type = ECesiumMetadataValueType::String;
    } else if (std::holds_alternative<float>(_value)) {
      _type = ECesiumMetadataValueType::Float;
    } else if (std::holds_alternative<double>(_value)) {
      _type = ECesiumMetadataValueType::String;
    } else if (std::holds_alternative<std::string_view>(_value)) {
      _type = ECesiumMetadataValueType::String;
    } else {
      _type = ECesiumMetadataValueType::Array;
    }
  }

  /**
   * Queries the underlying type of the property. For the most precise
   * representation of the property's values, you should retrieve them using
   * this type.
   *
   * @return The type of the property.
   */
  ECesiumMetadataValueType GetType() const;

  /**
   * Retrieves the value and attempts to convert it to a Boolean value.
   *
   * If the value is boolean, it is returned directly.
   *
   * If the value is numeric, zero is converted to false, while any other
   * value is converted to true.
   *
   * If the value is a string, "0", "false", and "no" (case-insensitive) are
   * converted to false, while "1", "true", and "yes" are converted to true.
   * All other strings, including strings that can be converted to numbers,
   * will return the default value.
   *
   * Other types of values will return the default value.
   *
   * @param DefaultValue The default value to use if the feature ID is invalid
   * or the feature's value cannot be converted.
   * @return The value.
   */
  bool GetBoolean(bool DefaultValue) const;

  /**
   * Retrieves the value and attempts to convert it to an unsigned 8-bit integer
   * value.
   *
   * If the value is an integer and between 0 and 255, it is returned
   * directly.
   *
   * If the value is a floating-point number in the range `(-1, 256)`, it is
   * truncated (rounded toward zero).
   *
   * If the value is a boolean, 0.0 is returned for false and 1.0 for true.
   *
   * If the value is a string and the entire string can be parsed as an
   * integer between 0 and 255, the parsed value is returned. The string is
   * parsed in a locale-independent way and does not support use of a comma or
   * other character to group digits.
   *
   * Otherwise, the default value is returned.
   *
   * @param DefaultValue The default value to use if the feature ID is invalid
   * or the feature's value cannot be converted.
   * @return The property value.
   */
  uint8 GetByte(uint8 DefaultValue) const;

  /**
   * Retrieves the value and attempts to convert it to a signed 32-bit integer
   * value.
   *
   * If the value is an integer and between -2,147,483,647 and 2,147,483,647,
   * it is returned directly.
   *
   * If the value is a floating-point number in the range
   * `(-2147483648, 2147483648)`, it is truncated (rounded toward zero).
   *
   * If the value is a boolean, 0 is returned for false and 1 for true.
   *
   * If the value is a string and the entire string can be parsed as an
   * integer in the valid range, the parsed value is returned. If it can be
   * parsed as a floating-point number, the parsed value is truncated (rounded
   * toward zero). In either case, the string is parsed in a locale-independent
   * way and does not support use of a comma or other character to group digits.
   *
   * Otherwise, the default value is returned.
   *
   * @param DefaultValue The default value to use if the feature ID is invalid
   * or the feature's value cannot be converted.
   * @return The property value.
   */
  int32 GetInteger(int32 DefaultValue) const;

  /**
   * Retrieves the value and attempts to convert it to a signed 64-bit integer
   * value.
   *
   * If the value is an integer and between -2^63-1 and 2^63-1,
   * it is returned directly.
   *
   * If the value is a floating-point number in the range `(-2^63, 2^63)`, it is
   * truncated (rounded toward zero).
   *
   * If the value is a boolean, 0 is returned for false and 1 for true.
   *
   * If the value is a string and the entire string can be parsed as an
   * integer in the valid range, the parsed value is returned. If it can be
   * parsed as a floating-point number, the parsed value is truncated (rounded
   * toward zero). In either case, the string is parsed in a locale-independent
   * way and does not support use of a comma or other character to group digits.
   *
   * Otherwise, the default value is returned.
   *
   * @param DefaultValue The default value to use if the feature ID is invalid
   * or the feature's value cannot be converted.
   * @return The property value.
   */
  int64 GetInteger64(int64 DefaultValue) const;

  /**
   * Retrieves the value and attempts to convert it to a 32-bit floating-point
   * value.
   *
   * If the value is a singe-precision floating-point number, is is returned.
   *
   * If the value is an integer or double-precision floating-point number,
   * it is converted to the closest representable single-precision
   * floating-point number.
   *
   * If the value is a boolean, 0.0 is returned for false and 1.0 for true.
   *
   * If the value is a string and the entire string can be parsed as a
   * number, the parsed value is returned. The string is parsed in a
   * locale-independent way and does not support use of a comma or other
   * character to group digits.
   *
   * Otherwise, the default value is returned.
   *
   * @param DefaultValue The default value to use if the feature ID is invalid
   * or the feature's value cannot be converted.
   * @return The property value.
   */
  float GetFloat(float DefaultValue) const;

  /**
   * Retrieves the value and attempts to convert it to a string value.
   *
   * A numeric value is converted to a string with `FString::Format`, which uses
   * the current locale.
   *
   * Boolean properties are converted to "true" or "false".
   *
   * Array properties return the `defaultValue`.
   *
   * String properties are returned directly.
   *
   * @param DefaultValue The default value to use if the feature ID is invalid
   * or the feature's value cannot be converted.
   * @return The property value.
   */
  FString GetString(const FString& DefaultValue) const;

  /**
   * Retrieves the value as an array.
   * If the property is not an array type, this method returns an empty array.
   *
   * @return The property value.
   */
  FCesiumMetadataArray GetArray() const;

private:
  ValueType _value;
  ECesiumMetadataValueType _type;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataGenericValueBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Queries the underlying type of the property. For the most precise
   * representation of the property's values, you should retrieve them using
   * this type.
   *
   * @return The type of the property.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|GenericValue")
  static ECesiumMetadataValueType
  GetType(UPARAM(ref) const FCesiumMetadataGenericValue& Value);

  /**
   * Retrieves the value and attempts to convert it to a Boolean value.
   *
   * If the value is boolean, it is returned directly.
   *
   * If the value is numeric, zero is converted to false, while any other
   * value is converted to true.
   *
   * If the value is a string, "0", "false", and "no" (case-insensitive) are
   * converted to false, while "1", "true", and "yes" are converted to true.
   * All other strings, including strings that can be converted to numbers,
   * will return the default value.
   *
   * Other types of values will return the default value.
   *
   * @param DefaultValue The default value to use if the feature ID is invalid
   * or the feature's value cannot be converted.
   * @return The value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|GenericValue")
  static bool GetBoolean(
      UPARAM(ref) const FCesiumMetadataGenericValue& value,
      bool DefaultValue);

  /**
   * Retrieves the value and attempts to convert it to an unsigned 8-bit integer
   * value.
   *
   * If the value is an integer and between 0 and 255, it is returned
   * directly.
   *
   * If the value is a floating-point number in the range `(-1, 256)`, it is
   * truncated (rounded toward zero).
   *
   * If the value is a boolean, 0.0 is returned for false and 1.0 for true.
   *
   * If the value is a string and the entire string can be parsed as an
   * integer between 0 and 255, the parsed value is returned. The string is
   * parsed in a locale-independent way and does not support use of a comma or
   * other character to group digits.
   *
   * Otherwise, the default value is returned.
   *
   * @param DefaultValue The default value to use if the feature ID is invalid
   * or the feature's value cannot be converted.
   * @return The property value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|GenericValue")
  static uint8 GetByte(
      UPARAM(ref) const FCesiumMetadataGenericValue& Value,
      uint8 DefaultValue);

  /**
   * Retrieves the value and attempts to convert it to a signed 32-bit integer
   * value.
   *
   * If the value is an integer and between -2,147,483,647 and 2,147,483,647,
   * it is returned directly.
   *
   * If the value is a floating-point number in the range
   * `(-2147483648, 2147483648)`, it is truncated (rounded toward zero).
   *
   * If the value is a boolean, 0 is returned for false and 1 for true.
   *
   * If the value is a string and the entire string can be parsed as an
   * integer in the valid range, the parsed value is returned. If it can be
   * parsed as a floating-point number, the parsed value is truncated (rounded
   * toward zero). In either case, the string is parsed in a locale-independent
   * way and does not support use of a comma or other character to group digits.
   *
   * Otherwise, the default value is returned.
   *
   * @param DefaultValue The default value to use if the feature ID is invalid
   * or the feature's value cannot be converted.
   * @return The property value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|GenericValue")
  static int32 GetInteger(
      UPARAM(ref) const FCesiumMetadataGenericValue& Value,
      int32 DefaultValue);

  /**
   * Retrieves the value and attempts to convert it to a signed 64-bit integer
   * value.
   *
   * If the value is an integer and between -2^63-1 and 2^63-1,
   * it is returned directly.
   *
   * If the value is a floating-point number in the range `(-2^63, 2^63)`, it is
   * truncated (rounded toward zero).
   *
   * If the value is a boolean, 0 is returned for false and 1 for true.
   *
   * If the value is a string and the entire string can be parsed as an
   * integer in the valid range, the parsed value is returned. If it can be
   * parsed as a floating-point number, the parsed value is truncated (rounded
   * toward zero). In either case, the string is parsed in a locale-independent
   * way and does not support use of a comma or other character to group digits.
   *
   * Otherwise, the default value is returned.
   *
   * @param DefaultValue The default value to use if the feature ID is invalid
   * or the feature's value cannot be converted.
   * @return The property value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|GenericValue")
  static int64 GetInteger64(
      UPARAM(ref) const FCesiumMetadataGenericValue& Value,
      int64 DefaultValue);

  /**
   * Retrieves the value and attempts to convert it to a 32-bit floating-point
   * value.
   *
   * If the value is a singe-precision floating-point number, is is returned.
   *
   * If the value is an integer or double-precision floating-point number,
   * it is converted to the closest representable single-precision
   * floating-point number.
   *
   * If the value is a boolean, 0.0 is returned for false and 1.0 for true.
   *
   * If the value is a string and the entire string can be parsed as a
   * number, the parsed value is returned. The string is parsed in a
   * locale-independent way and does not support use of a comma or other
   * character to group digits.
   *
   * Otherwise, the default value is returned.
   *
   * @param DefaultValue The default value to use if the feature ID is invalid
   * or the feature's value cannot be converted.
   * @return The property value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|GenericValue")
  static float GetFloat(
      UPARAM(ref) const FCesiumMetadataGenericValue& Value,
      float DefaultValue);

  /**
   * Retrieves the value and attempts to convert it to a string value.
   *
   * A numeric value is converted to a string with `FString::Format`, which uses
   * the current locale.
   *
   * Boolean properties are converted to "true" or "false".
   *
   * Array properties return the `defaultValue`.
   *
   * String properties are returned directly.
   *
   * @param DefaultValue The default value to use if the feature ID is invalid
   * or the feature's value cannot be converted.
   * @return The property value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|GenericValue")
  static FString GetString(
      UPARAM(ref) const FCesiumMetadataGenericValue& Value,
      const FString& DefaultValue);

  /**
   * Retrieves the value as an array.
   * If the property is not an array type, this method returns an empty array.
   *
   * @return The property value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|GenericValue")
  static FCesiumMetadataArray
  GetArray(UPARAM(ref) const FCesiumMetadataGenericValue& Value);
};
