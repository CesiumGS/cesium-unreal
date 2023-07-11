// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/PropertyArrayView.h"
#include "CesiumGltf/PropertyTypeTraits.h"
#include "CesiumMetadataArray.h"
#include "CesiumMetadataValueType.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ObjectMacros.h"
#include <glm/glm.hpp>
#include "CesiumMetadataValue.generated.h"

/**
 * A Blueprint-accessible wrapper for a glTF metadata value.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumMetadataValue {
  GENERATED_USTRUCT_BODY()

private:
#pragma region ValueType declaration
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
      glm::vec<2, int8_t>,
      glm::vec<2, uint8_t>,
      glm::vec<2, int16_t>,
      glm::vec<2, uint16_t>,
      glm::vec<2, int32_t>,
      glm::vec<2, uint32_t>,
      glm::vec<2, int64_t>,
      glm::vec<2, uint64_t>,
      glm::vec<2, float>,
      glm::vec<2, double>,
      glm::vec<3, int8_t>,
      glm::vec<3, uint8_t>,
      glm::vec<3, int16_t>,
      glm::vec<3, uint16_t>,
      glm::vec<3, int32_t>,
      glm::vec<3, uint32_t>,
      glm::vec<3, int64_t>,
      glm::vec<3, uint64_t>,
      glm::vec<3, float>,
      glm::vec<3, double>,
      glm::vec<4, int8_t>,
      glm::vec<4, uint8_t>,
      glm::vec<4, int16_t>,
      glm::vec<4, uint16_t>,
      glm::vec<4, int32_t>,
      glm::vec<4, uint32_t>,
      glm::vec<4, int64_t>,
      glm::vec<4, uint64_t>,
      glm::vec<4, float>,
      glm::vec<4, double>,
      glm::mat<2, 2, int8_t>,
      glm::mat<2, 2, uint8_t>,
      glm::mat<2, 2, int16_t>,
      glm::mat<2, 2, uint16_t>,
      glm::mat<2, 2, int32_t>,
      glm::mat<2, 2, uint32_t>,
      glm::mat<2, 2, int64_t>,
      glm::mat<2, 2, uint64_t>,
      glm::mat<2, 2, float>,
      glm::mat<2, 2, double>,
      glm::mat<3, 3, int8_t>,
      glm::mat<3, 3, uint8_t>,
      glm::mat<3, 3, int16_t>,
      glm::mat<3, 3, uint16_t>,
      glm::mat<3, 3, int32_t>,
      glm::mat<3, 3, uint32_t>,
      glm::mat<3, 3, int64_t>,
      glm::mat<3, 3, uint64_t>,
      glm::mat<3, 3, float>,
      glm::mat<3, 3, double>,
      glm::mat<4, 4, int8_t>,
      glm::mat<4, 4, uint8_t>,
      glm::mat<4, 4, int16_t>,
      glm::mat<4, 4, uint16_t>,
      glm::mat<4, 4, int32_t>,
      glm::mat<4, 4, uint32_t>,
      glm::mat<4, 4, int64_t>,
      glm::mat<4, 4, uint64_t>,
      glm::mat<4, 4, float>,
      glm::mat<4, 4, double>,
      CesiumGltf::PropertyArrayView<int8_t>,
      CesiumGltf::PropertyArrayView<uint8_t>,
      CesiumGltf::PropertyArrayView<int16_t>,
      CesiumGltf::PropertyArrayView<uint16_t>,
      CesiumGltf::PropertyArrayView<int32_t>,
      CesiumGltf::PropertyArrayView<uint32_t>,
      CesiumGltf::PropertyArrayView<int64_t>,
      CesiumGltf::PropertyArrayView<uint64_t>,
      CesiumGltf::PropertyArrayView<float>,
      CesiumGltf::PropertyArrayView<double>,
      CesiumGltf::PropertyArrayView<bool>,
      CesiumGltf::PropertyArrayView<std::string_view>,
      CesiumGltf::PropertyArrayView<glm::vec<2, int8_t>>,
      CesiumGltf::PropertyArrayView<glm::vec<2, uint8_t>>,
      CesiumGltf::PropertyArrayView<glm::vec<2, int16_t>>,
      CesiumGltf::PropertyArrayView<glm::vec<2, uint16_t>>,
      CesiumGltf::PropertyArrayView<glm::vec<2, int32_t>>,
      CesiumGltf::PropertyArrayView<glm::vec<2, uint32_t>>,
      CesiumGltf::PropertyArrayView<glm::vec<2, int64_t>>,
      CesiumGltf::PropertyArrayView<glm::vec<2, uint64_t>>,
      CesiumGltf::PropertyArrayView<glm::vec<2, float>>,
      CesiumGltf::PropertyArrayView<glm::vec<2, double>>,
      CesiumGltf::PropertyArrayView<glm::vec<3, int8_t>>,
      CesiumGltf::PropertyArrayView<glm::vec<3, uint8_t>>,
      CesiumGltf::PropertyArrayView<glm::vec<3, int16_t>>,
      CesiumGltf::PropertyArrayView<glm::vec<3, uint16_t>>,
      CesiumGltf::PropertyArrayView<glm::vec<3, int32_t>>,
      CesiumGltf::PropertyArrayView<glm::vec<3, uint32_t>>,
      CesiumGltf::PropertyArrayView<glm::vec<3, int64_t>>,
      CesiumGltf::PropertyArrayView<glm::vec<3, uint64_t>>,
      CesiumGltf::PropertyArrayView<glm::vec<3, float>>,
      CesiumGltf::PropertyArrayView<glm::vec<3, double>>,
      CesiumGltf::PropertyArrayView<glm::vec<4, int8_t>>,
      CesiumGltf::PropertyArrayView<glm::vec<4, uint8_t>>,
      CesiumGltf::PropertyArrayView<glm::vec<4, int16_t>>,
      CesiumGltf::PropertyArrayView<glm::vec<4, uint16_t>>,
      CesiumGltf::PropertyArrayView<glm::vec<4, int32_t>>,
      CesiumGltf::PropertyArrayView<glm::vec<4, uint32_t>>,
      CesiumGltf::PropertyArrayView<glm::vec<4, int64_t>>,
      CesiumGltf::PropertyArrayView<glm::vec<4, uint64_t>>,
      CesiumGltf::PropertyArrayView<glm::vec<4, float>>,
      CesiumGltf::PropertyArrayView<glm::vec<4, double>>,
      CesiumGltf::PropertyArrayView<glm::mat<2, 2, int8_t>>,
      CesiumGltf::PropertyArrayView<glm::mat<2, 2, uint8_t>>,
      CesiumGltf::PropertyArrayView<glm::mat<2, 2, int16_t>>,
      CesiumGltf::PropertyArrayView<glm::mat<2, 2, uint16_t>>,
      CesiumGltf::PropertyArrayView<glm::mat<2, 2, int32_t>>,
      CesiumGltf::PropertyArrayView<glm::mat<2, 2, uint32_t>>,
      CesiumGltf::PropertyArrayView<glm::mat<2, 2, int64_t>>,
      CesiumGltf::PropertyArrayView<glm::mat<2, 2, uint64_t>>,
      CesiumGltf::PropertyArrayView<glm::mat<2, 2, float>>,
      CesiumGltf::PropertyArrayView<glm::mat<2, 2, double>>,
      CesiumGltf::PropertyArrayView<glm::mat<3, 3, int8_t>>,
      CesiumGltf::PropertyArrayView<glm::mat<3, 3, uint8_t>>,
      CesiumGltf::PropertyArrayView<glm::mat<3, 3, int16_t>>,
      CesiumGltf::PropertyArrayView<glm::mat<3, 3, uint16_t>>,
      CesiumGltf::PropertyArrayView<glm::mat<3, 3, int32_t>>,
      CesiumGltf::PropertyArrayView<glm::mat<3, 3, uint32_t>>,
      CesiumGltf::PropertyArrayView<glm::mat<3, 3, int64_t>>,
      CesiumGltf::PropertyArrayView<glm::mat<3, 3, uint64_t>>,
      CesiumGltf::PropertyArrayView<glm::mat<3, 3, float>>,
      CesiumGltf::PropertyArrayView<glm::mat<3, 3, double>>,
      CesiumGltf::PropertyArrayView<glm::mat<4, 4, int8_t>>,
      CesiumGltf::PropertyArrayView<glm::mat<4, 4, uint8_t>>,
      CesiumGltf::PropertyArrayView<glm::mat<4, 4, int16_t>>,
      CesiumGltf::PropertyArrayView<glm::mat<4, 4, uint16_t>>,
      CesiumGltf::PropertyArrayView<glm::mat<4, 4, int32_t>>,
      CesiumGltf::PropertyArrayView<glm::mat<4, 4, uint32_t>>,
      CesiumGltf::PropertyArrayView<glm::mat<4, 4, int64_t>>,
      CesiumGltf::PropertyArrayView<glm::mat<4, 4, uint64_t>>,
      CesiumGltf::PropertyArrayView<glm::mat<4, 4, float>>,
      CesiumGltf::PropertyArrayView<glm::mat<4, 4, double>>>;
#pragma endregion

public:
  /**
   * Constructs an empty metadata value with unknown type.
   */
  FCesiumMetadataValue() : _value(std::monostate{}), _valueType() {}

  /**
   * Constructs a metadata value with the given input.
   *
   * @param Value The value to be stored in this struct.
   */
  template <typename T>
  explicit FCesiumMetadataValue(const T& Value) : _value(Value), _valueType() {
    ECesiumMetadataType type(CesiumGltf::TypeToPropertyType<T>::value);
    ECesiumMetadataComponentType componentType(
        CesiumGltf::TypeToPropertyType<T>::component);
    bool isArray = false;

    _valueType = {type, componentType, isArray};
  }

  /**
   * Constructs a metadata value with the given array input.
   *
   * @param Value The array value to be stored in this struct.
   */
  template <typename T>
  explicit FCesiumMetadataValue(const CesiumGltf::PropertyArrayView<T>& Value)
      : _value(Value), _valueType() {
    ECesiumMetadataType type(CesiumGltf::TypeToPropertyType<T>::value);
    ECesiumMetadataComponentType componentType(
        CesiumGltf::TypeToPropertyType<T>::component);
    bool isArray = true;

    _valueType = {type, componentType, isArray};
  }

private:
  ValueType _value;
  FCesiumMetadataValueType _valueType;

  friend class UCesiumMetadataValueBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataValueBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Gets the best-fitting Blueprints type for this value. For the most precise
   * representation of the value possible from Blueprints, you should retrieve
   * it using this type.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Value")
  static ECesiumMetadataBlueprintType
  GetBlueprintType(UPARAM(ref) const FCesiumMetadataValue& Value);

  /**
   * Gets the best-fitting Blueprints type for the elements of this array value.
   * If the given value is not of an array type, this returns None.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Value")
  static ECesiumMetadataBlueprintType
  GetArrayElementBlueprintType(UPARAM(ref) const FCesiumMetadataValue& Value);

  /**
   * Gets the true type of the metadata value as defined in the
   * EXT_structural_metadata extension. Many of these types are not accessible
   * from Blueprints, but can be converted to a Blueprint-accessible type.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Value")
  static FCesiumMetadataValueType
  GetValueType(UPARAM(ref) const FCesiumMetadataValue& Value);

  /**
   * Attempts to retrieve the value as a Boolean.
   *
   * If the value is a boolean, it is returned directly.
   *
   * If the value is a scalar, zero is converted to false, while any other
   * value is converted to true.
   *
   * If the value is a string, "0", "false", and "no" (case-insensitive) are
   * converted to false, while "1", "true", and "yes" are converted to true.
   * All other strings, including strings that can be converted to numbers,
   * will return the default value.
   *
   * All other types return the default value.
   *
   * @param DefaultValue The default value to use if the given value cannot
   * be converted to a Boolean.
   * @return The value as a Boolean.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Value")
  static bool
  GetBoolean(UPARAM(ref) const FCesiumMetadataValue& value, bool DefaultValue);

  /**
   * Attempts to retrieve the value as a Byte (unsigned 8-bit integer).
   *
   * If the value is an integer between 0 and 255, it is returned
   * directly.
   *
   * If the value is a floating-point number in the range `(-1, 256)`, it is
   * truncated (rounded toward zero).
   *
   * If the value is a boolean, 0 is returned for false and 1 for true.
   *
   * If the value is a string and the entire string can be parsed as an
   * integer between 0 and 255, the parsed value is returned. The string is
   * parsed in a locale-independent way and does not support use of a comma or
   * other delimiter to group digits.
   *
   * Otherwise, the default value is returned.
   *
   * @param DefaultValue The default value to use if the given value cannot
   * be converted to a Byte.
   * @return The value as a Byte.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Value")
  static uint8
  GetByte(UPARAM(ref) const FCesiumMetadataValue& Value, uint8 DefaultValue);

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
   * @param DefaultValue The default value to use if the metadata value cannot
   * be converted.
   * @return The value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Value")
  static int32
  GetInteger(UPARAM(ref) const FCesiumMetadataValue& Value, int32 DefaultValue);

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
   * @param DefaultValue The default value to use if the metadata value cannot
   * be converted.
   * @return The value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Value")
  static int64 GetInteger64(
      UPARAM(ref) const FCesiumMetadataValue& Value,
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
   * @param DefaultValue The default value to use if the metadata value cannot
   * be converted.
   * @return The value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Value")
  static float
  GetFloat(UPARAM(ref) const FCesiumMetadataValue& Value, float DefaultValue);

  /**
   * Gets the value and attempts to convert it to a 64-bit floating-point
   * value.
   *
   * If the value is a single- or double-precision floating-point number, is is
   * returned.
   *
   * If the value is an integer, it is converted to the closest representable
   * double-precision floating-point number.
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
   * @param DefaultValue The default value to use if the metadata value cannot
   * be converted.
   * @return The value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Value")
  static double GetFloat64(
      UPARAM(ref) const FCesiumMetadataValue& Value,
      double DefaultValue);

  ///**
  // * Gets the value and attempts to convert it to a Vector3f.
  // *
  // * If the value is a 3-dimensional vector with single-precision
  // floating-point
  // * components, it is returned.
  // *
  // * Otherwise, if the value is a 3-dimensional vector but with integer or
  // * double-precision components, each component will be converted to the
  // * closest representable single-precision floating-point number.
  // *
  // * Otherwise, if the value is a 4-dimensional vector, a 3-dimensional vector
  // * containing the first three components will be returned.
  // *
  // * If the value is a boolean, 0.0 is returned for false and 1.0 for true.
  // *
  // * If the value is a string and the entire string can be parsed as a
  // * number, the parsed value is returned. The string is parsed in a
  // * locale-independent way and does not support use of a comma or other
  // * character to group digits.
  // *
  // * Otherwise, the default value is returned.
  // *
  // * @param DefaultValue The default value to use if the metadata value cannot
  // * be converted.
  // * @return The value.
  // */
  // UFUNCTION(
  //    BlueprintCallable,
  //    BlueprintPure,
  //    Category = "Cesium|Metadata|Value")
  // static FVector3f GetVector3f(
  //    UPARAM(ref) const FCesiumMetadataValue& Value,
  //    FVector3f DefaultValue);

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
      Category = "Cesium|Metadata|Value")
  static FString GetString(
      UPARAM(ref) const FCesiumMetadataValue& Value,
      const FString& DefaultValue);

  /**
   * Gets the value as a metadata array. If the property is not an array type,
   * the returned array will be empty.
   *
   * @return The property value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Value")
  static FCesiumMetadataArray GetArray(UPARAM(ref)
                                           const FCesiumMetadataValue& Value);
};
