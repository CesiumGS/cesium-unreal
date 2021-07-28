// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/MetadataArrayView.h"
#include "CesiumGltf/PropertyTypeTraits.h"
#include "CesiumMetadataArray.h"
#include "CesiumMetadataValueType.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ObjectMacros.h"
#include "CesiumMetadataGenericValue.generated.h"

/**
 * A Blueprint-accessible wrapper for a glTF metadata value.
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
   * Constructs an empty value with unknown type.
   */
  FCesiumMetadataGenericValue()
      : _value(std::monostate{}), _type(ECesiumMetadataTrueType::None) {}

  /**
   * Constructs a value.
   *
   * @param Value The value to be stored in this struct
   */
  template <typename T>
  explicit FCesiumMetadataGenericValue(const T& Value)
      : _value(Value),
        _type(ECesiumMetadataTrueType::None),
        _componentType(ECesiumMetadataTrueType::None) {
    _type = ECesiumMetadataTrueType(CesiumGltf::TypeToPropertyType<T>::value);
    _componentType =
        ECesiumMetadataTrueType(CesiumGltf::TypeToPropertyType<T>::component);
  }

  /**
   * Gets best-fitting Blueprints type for the value. For the most precise
   * representation of the value possible from Blueprints, you should retrieve
   * it using this type.
   */
  ECesiumMetadataBlueprintType GetBlueprintType() const;

  /**
   * Gets best-fitting Blueprints type for the elements of this array. If this
   * value is not an array, returns None.
   */
  ECesiumMetadataBlueprintType GetBlueprintComponentType() const;

  /**
   * Gets true type of the value. Many of these types are not accessible
   * from Blueprints, but can be converted to a Blueprint-accessible type.
   */
  ECesiumMetadataTrueType GetTrueType() const;

  /**
   * Gets true type of the elements in the array. If this value is not an array,
   * the component type will be None. Many of these types are not accessible
   * from Blueprints, but can be converted to a Blueprint-accessible type.
   */
  ECesiumMetadataTrueType GetTrueComponentType() const;

  /**
   * Gets the value and attempts to convert it to a Boolean value.
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
   * @param DefaultValue The default value to use if the value cannot be
   * converted.
   * @return The value.
   */
  bool GetBoolean(bool DefaultValue) const;

  /**
   * Gets the value and attempts to convert it to an unsigned 8-bit integer
   * value.
   *
   * If the value is an integer and between 0 and 255, it is returned
   * directly.
   *
   * If the value is a floating-point number in the range `(-1, 256)`, it is
   * truncated (rounded toward zero).
   *
   * If the value is a boolean, 0 is returned for false and 1 for true.
   *
   * If the value is a string and the _entire_ string can be parsed as a
   * number between 0 and 255 (once truncated, if it's floating-point), the
   * parsed value is returned. The string is parsed in a locale-independent way
   * and does not support use of a comma or other character to group digits.
   *
   * Otherwise, the default value is returned.
   *
   * @param DefaultValue The default value to use if the value cannot be
   * converted.
   * @return The property value.
   */
  uint8 GetByte(uint8 DefaultValue) const;

  /**
   * Gets the value and attempts to convert it to a signed 32-bit integer
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
   * If the value is a string and the entire string can be parsed as a
   * number in the valid range (once truncated, if it's floating-point), the
   * parsed value is returned. In either case, the string is parsed in a
   * locale-independent way and does not support use of a comma or other
   * character to group digits.
   *
   * Otherwise, the default value is returned.
   *
   * @param DefaultValue The default value to use if the value cannot be
   * converted.
   * @return The property value.
   */
  int32 GetInteger(int32 DefaultValue) const;

  /**
   * Gets the value and attempts to convert it to a signed 64-bit integer
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
   * If the value is a string and the entire string can be parsed as a
   * number in the valid range (once truncated, if it's floating-point), the
   * parsed value is returned. In either case, the string is parsed in a
   * locale-independent way and does not support use of a comma or other
   * character to group digits.
   *
   * Otherwise, the default value is returned.
   *
   * @param DefaultValue The default value to use if the value cannot be
   * converted.
   * @return The property value.
   */
  int64 GetInteger64(int64 DefaultValue) const;

  /**
   * Gets the value and attempts to convert it to a 32-bit floating-point
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
   * @param DefaultValue The default value to use if the value cannot be
   * converted.
   * @return The property value.
   */
  float GetFloat(float DefaultValue) const;

  /**
   * Gets the value and attempts to convert it to a string value.
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
   * @param DefaultValue The default value to use if the value cannot be
   * converted.
   * @return The property value.
   */
  FString GetString(const FString& DefaultValue) const;

  /**
   * Gets the value as an array.
   * If the property is not an array type, this method returns an empty array.
   *
   * @return The property value.
   */
  FCesiumMetadataArray GetArray() const;

private:
  ValueType _value;
  ECesiumMetadataTrueType _type;
  ECesiumMetadataTrueType _componentType;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataGenericValueBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Gets best-fitting Blueprints type for the value. For the most precise
   * representation of the value possible from Blueprints, you should retrieve
   * it using this type.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|GenericValue")
  static ECesiumMetadataBlueprintType
  GetBlueprintType(UPARAM(ref) const FCesiumMetadataGenericValue& Value);

  /**
   * Gets best-fitting Blueprints type for the elements of this array. If this
   * value is not an array, returns None.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|GenericValue")
  static ECesiumMetadataBlueprintType
  GetBlueprintComponentType(UPARAM(ref)
                                const FCesiumMetadataGenericValue& Value);

  /**
   * Gets true type of the value. Many of these types are not accessible
   * from Blueprints, but can be converted to a Blueprint-accessible type.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|GenericValue")
  static ECesiumMetadataTrueType
  GetTrueType(UPARAM(ref) const FCesiumMetadataGenericValue& Value);

  /**
   * Gets true type of the elements in the array. If this value is not an array,
   * the component type will be None. Many of these types are not accessible
   * from Blueprints, but can be converted to a Blueprint-accessible type.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|GenericValue")
  static ECesiumMetadataTrueType
  GetTrueComponentType(UPARAM(ref) const FCesiumMetadataGenericValue& Value);

  /**
   * Gets the value and attempts to convert it to a Boolean value.
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
   * Gets the value and attempts to convert it to an unsigned 8-bit integer
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
   * Gets the value and attempts to convert it to a signed 32-bit integer
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
   * Gets the value and attempts to convert it to a signed 64-bit integer
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
   * Gets the value and attempts to convert it to a 32-bit floating-point
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
   * Gets the value and attempts to convert it to a string value.
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
   * Gets the value as an array.
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
