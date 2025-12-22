// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumMetadataEnum.h"
#include "CesiumMetadataValueType.h"
#include "CesiumPropertyArray.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ObjectMacros.h"

#include <CesiumGltf/PropertyTypeTraits.h>
#include <CesiumUtility/JsonValue.h>
#include <glm/glm.hpp>
#include <optional>
#include <swl/variant.hpp>

#include "CesiumMetadataValue.generated.h"

/**
 * A Blueprint-accessible wrapper for a glTF metadata value.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumMetadataValue {
  GENERATED_USTRUCT_BODY()

private:
#pragma region ValueType declaration
  using ValueType = swl::variant<
      swl::monostate,
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
      glm::mat<4, 4, double>>;
#pragma endregion

public:
  /**
   * Constructs an empty metadata value with unknown type.
   */
  FCesiumMetadataValue();

  /**
   * Constructs a metadata value with the given input.
   *
   * @param Value The value to be stored in this struct.
   * @param pEnumDefinition The enum definition for this metadata value, or
   * nullptr if not an enum.
   */
  template <typename T>
  explicit FCesiumMetadataValue(
      const T& Value,
      const TSharedPtr<FCesiumMetadataEnum>& pEnumDefinition)
      : _value(Value),
        _arrayValue(),
        _valueType(TypeToMetadataValueType<T>(pEnumDefinition)),
        _storage(),
        _pEnumDefinition(pEnumDefinition) {}

  /**
   * Constructs a metadata value with the given input.
   *
   * @param Value The value to be stored in this struct.
   */
  template <typename T>
  explicit FCesiumMetadataValue(const T& Value)
      : FCesiumMetadataValue(Value, TSharedPtr<FCesiumMetadataEnum>(nullptr)) {}

  template <typename ArrayType>
  explicit FCesiumMetadataValue(
      const CesiumGltf::PropertyArrayView<ArrayType>& Value,
      const TSharedPtr<FCesiumMetadataEnum>& pEnumDefinition = nullptr)
      : FCesiumMetadataValue(
            CesiumGltf::PropertyArrayView<ArrayType>(Value),
            pEnumDefinition) {}

  template <typename ArrayType>
  explicit FCesiumMetadataValue(
      CesiumGltf::PropertyArrayView<ArrayType>&& Value,
      const TSharedPtr<FCesiumMetadataEnum>& pEnumDefinition = nullptr)
      : _value(),
        _arrayValue(FCesiumPropertyArray(std::move(Value))),
        _valueType(
            TypeToMetadataValueType<CesiumGltf::PropertyArrayView<ArrayType>>(
                pEnumDefinition)),
        _storage(),
        _pEnumDefinition(pEnumDefinition) {}

  template <typename ArrayType>
  explicit FCesiumMetadataValue(
      const CesiumGltf::PropertyArrayCopy<ArrayType>& Value,
      const TSharedPtr<FCesiumMetadataEnum>& pEnumDefinition = nullptr)
      : FCesiumMetadataValue(
            CesiumGltf::PropertyArrayCopy<ArrayType>(Value),
            pEnumDefinition) {}

  template <typename ArrayType>
  explicit FCesiumMetadataValue(
      CesiumGltf::PropertyArrayCopy<ArrayType>&& Value,
      const TSharedPtr<FCesiumMetadataEnum>& pEnumDefinition = nullptr)
      : _value(),
        _arrayValue(FCesiumPropertyArray(std::move(Value))),
        _valueType(
            TypeToMetadataValueType<CesiumGltf::PropertyArrayView<ArrayType>>(
                pEnumDefinition)),
        _storage(),
        _pEnumDefinition(pEnumDefinition) {}

  /**
   * Constructs a metadata value with the given optional input.
   *
   * @param MaybeValue The optional value to be stored in this struct.
   * @param pEnumDefinition The enum definition for this metadata value, or
   * nullptr if not an enum.
   */
  template <typename T>
  explicit FCesiumMetadataValue(
      const std::optional<T>& MaybeValue,
      const TSharedPtr<FCesiumMetadataEnum>& pEnumDefinition = nullptr)
      : _value(),
        _arrayValue(),
        _valueType(),
        _pEnumDefinition(pEnumDefinition) {
    if (!MaybeValue) {
      return;
    }

    FCesiumMetadataValue temp(*MaybeValue);
    this->_value = std::move(temp._value);
    this->_arrayValue = std::move(temp._arrayValue);
    this->_valueType = std::move(temp._valueType);
  }

  /**
   * Constructs a metadata value from a given FString.
   */
  FCesiumMetadataValue(const FString& String);

  /**
   * Constructs a metadata value from a given FCesiumPropertyArray.
   */
  FCesiumMetadataValue(FCesiumPropertyArray&& Array);

  FCesiumMetadataValue(FCesiumMetadataValue&& rhs);
  FCesiumMetadataValue& operator=(FCesiumMetadataValue&& rhs);
  FCesiumMetadataValue(const FCesiumMetadataValue& rhs);
  FCesiumMetadataValue& operator=(const FCesiumMetadataValue& rhs);

  /**
   * Converts a CesiumUtility::JsonValue to a FCesiumMetadataValue with the
   * specified type. This is a strict interpretation of the value; the function
   * will not convert between types or component types, even if possible.
   *
   * @param jsonValue The JSON value.
   * @param targetType The value type to which to convert the JSON value.
   * @returns The value as an FCesiumMetadataValue.
   */
  static FCesiumMetadataValue fromJsonValue(
      const CesiumUtility::JsonValue& jsonValue,
      const FCesiumMetadataValueType& targetType);

private:
  /**
   * Create a FCesiumMetadataValue from the given JsonValue::Array, interpreted
   * from the target type.
   *
   * Numeric arrays can be interpreted as either scalar arrays or vector/matrix
   * values. It is also possible to have arrays of vectors or arrays of
   * matrices. If this function is called with bIsArray = true, then nested
   * arrays are acceptable (assuming vector or matrix type). Otherwise, the
   * array is considered invalid.
   */
  static FCesiumMetadataValue fromJsonArray(
      const CesiumUtility::JsonValue::Array& jsonValue,
      const FCesiumMetadataValueType& targetType);

  ValueType _value;
  std::optional<FCesiumPropertyArray> _arrayValue;

  FCesiumMetadataValueType _valueType;
  std::vector<std::byte> _storage;
  TSharedPtr<FCesiumMetadataEnum> _pEnumDefinition;

  friend class UCesiumMetadataValueBlueprintLibrary;
  friend class CesiumMetadataValueAccess;
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
   * Gets the type of the metadata value as defined in the
   * EXT_structural_metadata extension. Many of these types are not accessible
   * from Blueprints, but can be converted to a Blueprint-accessible type.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Value")
  static FCesiumMetadataValueType
  GetValueType(UPARAM(ref) const FCesiumMetadataValue& Value);

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  /**
   * Gets true type of the value. Many of these types are not accessible
   * from Blueprints, but can be converted to a Blueprint-accessible type.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "CesiumMetadataTrueType is deprecated. Use GetValueType to get the CesiumMetadataValueType instead."))
  static ECesiumMetadataTrueType_DEPRECATED
  GetTrueType(UPARAM(ref) const FCesiumMetadataValue& Value);

  /**
   * Gets true type of the elements in the array. If this value is not an array,
   * the component type will be None. Many of these types are not accessible
   * from Blueprints, but can be converted to a Blueprint-accessible type.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "CesiumMetadataTrueType is deprecated. Use GetValueType to get the CesiumMetadataValueType instead."))
  static ECesiumMetadataTrueType_DEPRECATED
  GetTrueComponentType(UPARAM(ref) const FCesiumMetadataValue& Value);

  PRAGMA_ENABLE_DEPRECATION_WARNINGS

  /**
   * Attempts to retrieve the value as a boolean.
   *
   * If the value is a boolean, it is returned as-is.
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
   * @param value The metadata value to retrieve.
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
   * Attempts to retrieve the value as an unsigned 8-bit integer.
   *
   * If the value is an integer between 0 and 255, it is returned
   * as-is.
   *
   * If the value is a floating-point number in the aforementioned range, it is
   * truncated (rounded toward zero) and returned.
   *
   * If the value is a boolean, 1 is returned for true and 0 for false.
   *
   * If the value is a string and the entire string can be parsed as an
   * integer between 0 and 255, the parsed value is returned. The string is
   * parsed in a locale-independent way and does not support the use of commas
   * or other delimiters to group digits together.
   *
   * In all other cases, the default value is returned.
   *
   * @param Value The metadata value to retrieve.
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
   * Attempts to retrieve the value as a signed 32-bit integer.
   *
   * If the value is an integer between -2,147,483,648 and 2,147,483,647,
   * it is returned as-is.
   *
   * If the value is a floating-point number in the aforementioned range, it is
   * truncated (rounded toward zero) and returned;
   *
   * If the value is a boolean, 1 is returned for true and 0 for false.
   *
   * If the value is a string and the entire string can be parsed as an
   * integer in the valid range, the parsed value is returned. If it can be
   * parsed as a floating-point number, the parsed value is truncated (rounded
   * toward zero). In either case, the string is parsed in a locale-independent
   * way and does not support the use of commas or other delimiters to group
   * digits together.
   *
   * In all other cases, the default value is returned.
   *
   * @param Value The metadata value to retrieve.
   * @param DefaultValue The default value to use if the given value cannot
   * be converted to an Integer.
   * @return The value as an Integer.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Value")
  static int32
  GetInteger(UPARAM(ref) const FCesiumMetadataValue& Value, int32 DefaultValue);

  /**
   * Attempts to retrieve the value as a signed 64-bit integer.
   *
   * If the value is an integer and between -2^63 and (2^63 - 1),
   * it is returned as-is.
   *
   * If the value is a floating-point number in the aforementioned range, it
   * is truncated (rounded toward zero) and returned;
   *
   * If the value is a boolean, 1 is returned for true and 0 for false.
   *
   * If the value is a string and the entire string can be parsed as an
   * integer in the valid range, the parsed value is returned. If it can be
   * parsed as a floating-point number, the parsed value is truncated (rounded
   * toward zero). In either case, the string is parsed in a locale-independent
   * way and does not support the use of commas or other delimiters to group
   * digits together.
   *
   * In all other cases, the default value is returned.
   *
   * @param Value The metadata value to retrieve.
   * @param DefaultValue The default value to use if the given value cannot
   * be converted to an Integer64.
   * @return The value as an Integer64.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Value")
  static int64 GetInteger64(
      UPARAM(ref) const FCesiumMetadataValue& Value,
      int64 DefaultValue);

  /**
   * Attempts to retrieve the value as a single-precision floating-point number.
   *
   * If the value is already a single-precision floating-point number, it is
   * returned as-is.
   *
   * If the value is a scalar of any other type within the range of values that
   * a single-precision float can represent, it is converted to its closest
   * representation as a single-precision float and returned.
   *
   * If the value is a boolean, 1.0f is returned for true and 0.0f for false.
   *
   * If the value is a string, and the entire string can be parsed as a
   * number, the parsed value is returned. The string is parsed in a
   * locale-independent way and does not support the use of a comma or other
   * delimiter to group digits togther.
   *
   * In all other cases, the default value is returned.
   *
   * @param Value The metadata value to retrieve.
   * @param DefaultValue The default value to use if the given value cannot
   * be converted to a Float.
   * @return The value as a Float.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Value")
  static float
  GetFloat(UPARAM(ref) const FCesiumMetadataValue& Value, float DefaultValue);

  /**
   * Attempts to retrieve the value as a double-precision floating-point number.
   *
   * If the value is a single- or double-precision floating-point number, it is
   * returned as-is.
   *
   * If the value is an integer, it is converted to the closest representable
   * double-precision floating-point number.
   *
   * If the value is a boolean, 1.0 is returned for true and 0.0 for false.
   *
   * If the value is a string and the entire string can be parsed as a
   * number, the parsed value is returned. The string is parsed in a
   * locale-independent way and does not support the use of commas or other
   * delimiters to group digits together.
   *
   * In all other cases, the default value is returned.
   *
   * @param Value The metadata value to retrieve.
   * @param DefaultValue The default value to use if the given value cannot
   * be converted to a Float64.
   * @return The value as a Float64.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Value")
  static double GetFloat64(
      UPARAM(ref) const FCesiumMetadataValue& Value,
      double DefaultValue);

  /**
   * Attempts to retrieve the value as a FIntPoint.
   *
   * If the value is a 2-dimensional vector, its components will be converted to
   * 32-bit signed integers if possible.
   *
   * If the value is a 3- or 4-dimensional vector, it will use the first two
   * components to construct the FIntPoint.
   *
   * If the value is a scalar that can be converted to a 32-bit signed integer,
   * the resulting FIntPoint will have this value in both of its components.
   *
   * If the value is a boolean, (1, 1) is returned for true, while (0, 0) is
   * returned for false.
   *
   * If the value is a string that can be parsed as a FIntPoint, the parsed
   * value is returned. The string must be formatted as "X=... Y=...".
   *
   * In all other cases, the default value is returned. In all vector cases, if
   * any of the relevant components cannot be represented as a 32-bit signed,
   * the default value is returned.
   *
   * @param Value The metadata value to retrieve.
   * @param DefaultValue The default value to use if the given value cannot
   * be converted to a FIntPoint.
   * @return The value as a FIntPoint.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Value")
  static FIntPoint GetIntPoint(
      UPARAM(ref) const FCesiumMetadataValue& Value,
      const FIntPoint& DefaultValue);

  /**
   * Attempts to retrieve the value as a FVector2D.
   *
   * If the value is a 2-dimensional vector, its components will be converted to
   * double-precision floating-point numbers.
   *
   * If the value is a 3- or 4-dimensional vector, it will use the first two
   * components to construct the FVector2D.
   *
   * If the value is a scalar, the resulting FVector2D will have this value in
   * both of its components.
   *
   * If the value is a boolean, (1.0, 1.0) is returned for true, while (0.0,
   * 0.0) is returned for false.
   *
   * If the value is a string that can be parsed as a FVector2D, the parsed
   * value is returned. The string must be formatted as "X=... Y=...".
   *
   * In all other cases, the default value is returned.
   *
   * @param Value The metadata value to retrieve.
   * @param DefaultValue The default value to use if the given value cannot
   * be converted to a FIntPoint.
   * @return The value as a FIntPoint.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Value")
  static FVector2D GetVector2D(
      UPARAM(ref) const FCesiumMetadataValue& Value,
      const FVector2D& DefaultValue);

  /**
   * Attempts to retrieve the value as a FIntVector.
   *
   * If the value is a 3-dimensional vector, its components will be converted to
   * 32-bit signed integers if possible.
   *
   * If the value is a 4-dimensional vector, it will use the first three
   * components to construct the FIntVector.
   *
   * If the value is a 2-dimensional vector, it will become the XY-components of
   * the FIntVector. The Z component will be set to zero.
   *
   * If the value is a scalar that can be converted to a 32-bit signed integer,
   * the resulting FIntVector will have this value in all of its components.
   *
   * If the value is a boolean, (1, 1, 1) is returned for true, while (0, 0, 0)
   * is returned for false.
   *
   * If the value is a string that can be parsed as a FIntVector, the parsed
   * value is returned. The string must be formatted as "X=... Y=... Z=".
   *
   * In all other cases, the default value is returned. In all vector cases, if
   * any of the relevant components cannot be represented as a 32-bit signed
   * integer, the default value is returned.
   *
   * @param Value The metadata value to retrieve.
   * @param DefaultValue The default value to use if the given value cannot
   * be converted to a FIntVector.
   * @return The value as a FIntVector.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Value")
  static FIntVector GetIntVector(
      UPARAM(ref) const FCesiumMetadataValue& Value,
      const FIntVector& DefaultValue);

  /**
   * Attempts to retrieve the value as a FVector3f.
   *
   * If the value is a 3-dimensional vector, its components will be converted to
   * the closest representable single-precision floats, if possible.
   *
   * If the value is a 4-dimensional vector, a FVector3f containing the first
   * three components will be returned.
   *
   * If the value is a 2-dimensional vector, it will become the XY-components of
   * the FVector3f. The Z-component will be set to zero.
   *
   * If the value is a scalar that can be converted to a single-precision
   * floating-point number, then the resulting FVector3f will have this value in
   * all of its components.
   *
   * If the value is a boolean, (1.0f, 1.0f, 1.0f) is returned for true, while
   * (0.0f, 0.0f, 0.0f) is returned for false.
   *
   * If the value is a string that can be parsed as a FVector3f, the parsed
   * value is returned. The string must be formatted as "X=... Y=... Z=".
   *
   * In all other cases, the default value is returned. In all vector cases, if
   * any of the relevant components cannot be represented as a single-precision
   * float, the default value is returned.
   *
   * @param Value The metadata value to retrieve.
   * @param DefaultValue The default value to use if the given value cannot
   * be converted to a FVector3f.
   * @return The value as a FVector3f.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Value")
  static FVector3f GetVector3f(
      UPARAM(ref) const FCesiumMetadataValue& Value,
      const FVector3f& DefaultValue);

  /**
   * Attempts to retrieve the value as a FVector.
   *
   * If the value is a 3-dimensional vector, its components will be converted to
   * double-precision floating-point numbers.
   *
   * If the value is a 4-dimensional vector, a FVector containing the first
   * three components will be returned.
   *
   * If the value is a 2-dimensional vector, it will become the XY-components of
   * the FVector. The Z-component will be set to zero.
   *
   * If the value is a scalar, then the resulting FVector will have this value
   * as a double-precision floating-point number in all of its components.
   *
   * If the value is a boolean, (1.0, 1.0, 1.0) is returned for true, while
   * (0.0, 0.0, 0.0) is returned for false.
   *
   * If the value is a string that can be parsed as a FVector, the parsed
   * value is returned. The string must be formatted as "X=... Y=... Z=".
   *
   * In all other cases, the default value is returned.
   *
   * @param Value The metadata value to retrieve.
   * @param DefaultValue The default value to use if the given value cannot
   * be converted to a FVector.
   * @return The value as a FVector.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Value")
  static FVector GetVector(
      UPARAM(ref) const FCesiumMetadataValue& Value,
      const FVector& DefaultValue);

  /**
   * Attempts to retrieve the value as a FVector4.
   *
   * If the value is a 4-dimensional vector, its components will be converted to
   * double-precision floating-point numbers.
   *
   * If the value is a 3-dimensional vector, it will become the XYZ-components
   * of the FVector4. The W-component will be set to zero.
   *
   * If the value is a 2-dimensional vector, it will become the XY-components of
   * the FVector4. The Z- and W-components will be set to zero.
   *
   * If the value is a scalar, then the resulting FVector4 will have this value
   * as a double-precision floating-point number in all of its components.
   *
   * If the value is a boolean, (1.0, 1.0, 1.0, 1.0) is returned for true, while
   * (0.0, 0.0, 0.0, 0.0) is returned for false.
   *
   * If the value is a string that can be parsed as a FVector4, the parsed
   * value is returned. This follows the rules of FVector4::InitFromString. The
   * string must be formatted as "X=... Y=... Z=... W=...". The W-component is
   * optional; if absent, it will be set to 1.0.
   *
   * In all other cases, the default value is returned.
   *
   * @param Value The metadata value to retrieve.
   * @param DefaultValue The default value to use if the given value cannot
   * be converted to a FVector4.
   * @return The value as a FVector4.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Value")
  static FVector4 GetVector4(
      UPARAM(ref) const FCesiumMetadataValue& Value,
      const FVector4& DefaultValue);

  /**
   * Attempts to retrieve the value as a FMatrix.
   *
   * If the value is a 4-by-4 matrix, its components will be converted to
   * double-precision floating-point numbers.
   *
   * If the value is a 3-by-3 matrix, it will initialize the corresponding
   * entries of the FMatrix, while all other entries are set to zero. In other
   * words, the 3-by-3 matrix is returned in an FMatrix where the fourth row and
   * column are filled with zeroes.
   *
   * If the value is a 2-by-2 matrix, it will initialize the corresponding
   * entries of the FMatrix, while all other entries are set to zero. In other
   * words, the 2-by-2 matrix is returned in an FMatrix where the third and
   * fourth rows / columns are filled with zeroes.
   *
   * If the value is a scalar, then the resulting FMatrix will have this value
   * along its diagonal, including the very last component. All other entries
   * will be zero.
   *
   * If the value is a boolean, it is converted to 1.0 for true and 0.0 for
   * false. Then, the resulting FMatrix will have this value along its diagonal,
   * including the very last component. All other entries will be zero.
   *
   * In all other cases, the default value is returned.
   *
   * @param Value The metadata value to retrieve.
   * @param DefaultValue The default value to use if the given value cannot
   * be converted to a FMatrix.
   * @return The value as a FMatrix.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Value")
  static FMatrix GetMatrix(
      UPARAM(ref) const FCesiumMetadataValue& Value,
      const FMatrix& DefaultValue);

  /**
   * Attempts to retrieve the value as a FString.
   *
   * String properties are returned as-is.
   *
   * Scalar values are converted to a string with `std::to_string`.
   *
   * Boolean properties are converted to "true" or "false".
   *
   * Vector properties are returned as strings in the format "X=... Y=... Z=...
   * W=..." depending on how many components they have.
   *
   * Matrix properties are returned as strings row-by-row, where each row's
   * values are printed between square brackets. For example, a 2-by-2 matrix
   * will be printed out as "[A B] [C D]".
   *
   * Array properties return the default value.
   *
   * @param Value The metadata value to retrieve.
   * @param DefaultValue The default value to use if the given value cannot
   * be converted to a FString.
   * @return The value as a FString.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Value")
  static FString GetString(
      UPARAM(ref) const FCesiumMetadataValue& Value,
      const FString& DefaultValue);

  /**
   * Attempts to retrieve the value as a FCesiumPropertyArray. If the property
   * is not an array type, this returns an empty array.
   *
   * @param Value The metadata value to retrieve.
   * @return The value as a FCesiumPropertyArray.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Value")
  static FCesiumPropertyArray GetArray(UPARAM(ref)
                                           const FCesiumMetadataValue& Value);

  /**
   * Whether the value is empty, i.e., whether it does not actually represent
   * any data. An empty value functions as a null value, and can be compared to
   * a std::nullopt in C++. For example, when the raw value of a property
   * matches the property's specified "no data" value, it will return an empty
   * FCesiumMetadataValue.
   *
   * @param Value The metadata value to retrieve.
   * @return Whether the value is empty.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Value")
  static bool IsEmpty(UPARAM(ref) const FCesiumMetadataValue& Value);

  /**
   * Gets the given map of metadata values as a new map of strings, mapped by
   * name. This is useful for displaying the values from a property table or
   * property texture as strings in a user interface.
   *
   * Array properties cannot be converted to strings, so empty strings
   * will be returned for their values.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Value")
  static TMap<FString, FString>
  GetValuesAsStrings(const TMap<FString, FCesiumMetadataValue>& Values);
};

/**
 * Grants access to metadata value types that are not currently supported in
 * Blueprints. This can be useful in C++ code.
 *
 * These should be moved to UCesiumMetadataValueBlueprintLibrary if those types
 * become compatible with Blueprints in the future.
 */
class CESIUMRUNTIME_API CesiumMetadataValueAccess {

public:
  /**
   * Attempts to retrieve the value as an unsigned 64-bit integer.
   *
   * If the value is an integer and between 0 and (2^64 - 1),
   * it is returned as-is.
   *
   * If the value is a floating-point number in the aforementioned range, it
   * is truncated (rounded toward zero) and returned;
   *
   * If the value is a boolean, 1 is returned for true and 0 for false.
   *
   * If the value is a string and the entire string can be parsed as an
   * integer in the valid range, the parsed value is returned. If it can be
   * parsed as a floating-point number, the parsed value is truncated (rounded
   * toward zero). In either case, the string is parsed in a locale-independent
   * way and does not support the use of commas or other delimiters to group
   * digits together.
   *
   * In all other cases, the default value is returned.
   *
   * @param DefaultValue The default value to use if the given value cannot
   * be converted to an uint64.
   * @return The value as an unsigned 64-bit integer.
   */
  static uint64
  GetUnsignedInteger64(const FCesiumMetadataValue& Value, uint64 DefaultValue);
};
