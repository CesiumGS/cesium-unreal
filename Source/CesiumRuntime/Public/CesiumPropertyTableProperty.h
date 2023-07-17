// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/PropertyTablePropertyView.h"
#include "CesiumGltf/PropertyTypeTraits.h"
#include "CesiumMetadataValue.h"
#include "CesiumMetadataValueType.h"
#include "CesiumPropertyArray.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ObjectMacros.h"
#include <glm/glm.hpp>
#include <string_view>
#include <variant>
#include "CesiumPropertyTableProperty.generated.h"

/**
 * @brief Reports the status of a FCesiumPropertyTableProperty. If the property
 * table property cannot be accessed, this briefly indicates why.
 */
UENUM(BlueprintType)
enum class ECesiumPropertyTablePropertyStatus : uint8 {
  /* The property table property is valid. */
  Valid = 0,
  /* The property table property does not exist in the glTF, or the property
     definition itself contains errors. */
  ErrorInvalidProperty,
  /* The data associated with the property table property is malformed and
     cannot be retrieved. */
  ErrorInvalidPropertyData
};

/**
 * A Blueprint-accessible wrapper for a glTF property table property in
 * EXT_structural_metadata. A property has a specific type, such as int64 scalar
 * or string, and values of that type that can be accessed with primitive
 * feature IDs from EXT_mesh_features.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumPropertyTableProperty {
  GENERATED_USTRUCT_BODY()

private:
#pragma region PropertyType declaration
  template <typename T>
  using PropertyView = CesiumGltf::PropertyTablePropertyView<T>;

  template <typename T>
  using ArrayPropertyView =
      CesiumGltf::PropertyTablePropertyView<CesiumGltf::PropertyArrayView<T>>;

  using PropertyType = std::variant<
      PropertyView<int8_t>,
      PropertyView<uint8_t>,
      PropertyView<int16_t>,
      PropertyView<uint16_t>,
      PropertyView<int32_t>,
      PropertyView<uint32_t>,
      PropertyView<int64_t>,
      PropertyView<uint64_t>,
      PropertyView<float>,
      PropertyView<double>,
      PropertyView<bool>,
      PropertyView<std::string_view>,
      PropertyView<glm::vec<2, int8_t>>,
      PropertyView<glm::vec<2, uint8_t>>,
      PropertyView<glm::vec<2, int16_t>>,
      PropertyView<glm::vec<2, uint16_t>>,
      PropertyView<glm::vec<2, int32_t>>,
      PropertyView<glm::vec<2, uint32_t>>,
      PropertyView<glm::vec<2, int64_t>>,
      PropertyView<glm::vec<2, uint64_t>>,
      PropertyView<glm::vec<2, float>>,
      PropertyView<glm::vec<2, double>>,
      PropertyView<glm::vec<3, int8_t>>,
      PropertyView<glm::vec<3, uint8_t>>,
      PropertyView<glm::vec<3, int16_t>>,
      PropertyView<glm::vec<3, uint16_t>>,
      PropertyView<glm::vec<3, int32_t>>,
      PropertyView<glm::vec<3, uint32_t>>,
      PropertyView<glm::vec<3, int64_t>>,
      PropertyView<glm::vec<3, uint64_t>>,
      PropertyView<glm::vec<3, float>>,
      PropertyView<glm::vec<3, double>>,
      PropertyView<glm::vec<4, int8_t>>,
      PropertyView<glm::vec<4, uint8_t>>,
      PropertyView<glm::vec<4, int16_t>>,
      PropertyView<glm::vec<4, uint16_t>>,
      PropertyView<glm::vec<4, int32_t>>,
      PropertyView<glm::vec<4, uint32_t>>,
      PropertyView<glm::vec<4, int64_t>>,
      PropertyView<glm::vec<4, uint64_t>>,
      PropertyView<glm::vec<4, float>>,
      PropertyView<glm::vec<4, double>>,
      PropertyView<glm::mat<2, 2, int8_t>>,
      PropertyView<glm::mat<2, 2, uint8_t>>,
      PropertyView<glm::mat<2, 2, int16_t>>,
      PropertyView<glm::mat<2, 2, uint16_t>>,
      PropertyView<glm::mat<2, 2, int32_t>>,
      PropertyView<glm::mat<2, 2, uint32_t>>,
      PropertyView<glm::mat<2, 2, int64_t>>,
      PropertyView<glm::mat<2, 2, uint64_t>>,
      PropertyView<glm::mat<2, 2, float>>,
      PropertyView<glm::mat<2, 2, double>>,
      PropertyView<glm::mat<3, 3, int8_t>>,
      PropertyView<glm::mat<3, 3, uint8_t>>,
      PropertyView<glm::mat<3, 3, int16_t>>,
      PropertyView<glm::mat<3, 3, uint16_t>>,
      PropertyView<glm::mat<3, 3, int32_t>>,
      PropertyView<glm::mat<3, 3, uint32_t>>,
      PropertyView<glm::mat<3, 3, int64_t>>,
      PropertyView<glm::mat<3, 3, uint64_t>>,
      PropertyView<glm::mat<3, 3, float>>,
      PropertyView<glm::mat<3, 3, double>>,
      PropertyView<glm::mat<4, 4, int8_t>>,
      PropertyView<glm::mat<4, 4, uint8_t>>,
      PropertyView<glm::mat<4, 4, int16_t>>,
      PropertyView<glm::mat<4, 4, uint16_t>>,
      PropertyView<glm::mat<4, 4, int32_t>>,
      PropertyView<glm::mat<4, 4, uint32_t>>,
      PropertyView<glm::mat<4, 4, int64_t>>,
      PropertyView<glm::mat<4, 4, uint64_t>>,
      PropertyView<glm::mat<4, 4, float>>,
      PropertyView<glm::mat<4, 4, double>>,
      ArrayPropertyView<int8_t>,
      ArrayPropertyView<uint8_t>,
      ArrayPropertyView<int16_t>,
      ArrayPropertyView<uint16_t>,
      ArrayPropertyView<int32_t>,
      ArrayPropertyView<uint32_t>,
      ArrayPropertyView<int64_t>,
      ArrayPropertyView<uint64_t>,
      ArrayPropertyView<float>,
      ArrayPropertyView<double>,
      ArrayPropertyView<bool>,
      ArrayPropertyView<std::string_view>,
      ArrayPropertyView<glm::vec<2, int8_t>>,
      ArrayPropertyView<glm::vec<2, uint8_t>>,
      ArrayPropertyView<glm::vec<2, int16_t>>,
      ArrayPropertyView<glm::vec<2, uint16_t>>,
      ArrayPropertyView<glm::vec<2, int32_t>>,
      ArrayPropertyView<glm::vec<2, uint32_t>>,
      ArrayPropertyView<glm::vec<2, int64_t>>,
      ArrayPropertyView<glm::vec<2, uint64_t>>,
      ArrayPropertyView<glm::vec<2, float>>,
      ArrayPropertyView<glm::vec<2, double>>,
      ArrayPropertyView<glm::vec<3, int8_t>>,
      ArrayPropertyView<glm::vec<3, uint8_t>>,
      ArrayPropertyView<glm::vec<3, int16_t>>,
      ArrayPropertyView<glm::vec<3, uint16_t>>,
      ArrayPropertyView<glm::vec<3, int32_t>>,
      ArrayPropertyView<glm::vec<3, uint32_t>>,
      ArrayPropertyView<glm::vec<3, int64_t>>,
      ArrayPropertyView<glm::vec<3, uint64_t>>,
      ArrayPropertyView<glm::vec<3, float>>,
      ArrayPropertyView<glm::vec<3, double>>,
      ArrayPropertyView<glm::vec<4, int8_t>>,
      ArrayPropertyView<glm::vec<4, uint8_t>>,
      ArrayPropertyView<glm::vec<4, int16_t>>,
      ArrayPropertyView<glm::vec<4, uint16_t>>,
      ArrayPropertyView<glm::vec<4, int32_t>>,
      ArrayPropertyView<glm::vec<4, uint32_t>>,
      ArrayPropertyView<glm::vec<4, int64_t>>,
      ArrayPropertyView<glm::vec<4, uint64_t>>,
      ArrayPropertyView<glm::vec<4, float>>,
      ArrayPropertyView<glm::vec<4, double>>,
      ArrayPropertyView<glm::mat<2, 2, int8_t>>,
      ArrayPropertyView<glm::mat<2, 2, uint8_t>>,
      ArrayPropertyView<glm::mat<2, 2, int16_t>>,
      ArrayPropertyView<glm::mat<2, 2, uint16_t>>,
      ArrayPropertyView<glm::mat<2, 2, int32_t>>,
      ArrayPropertyView<glm::mat<2, 2, uint32_t>>,
      ArrayPropertyView<glm::mat<2, 2, int64_t>>,
      ArrayPropertyView<glm::mat<2, 2, uint64_t>>,
      ArrayPropertyView<glm::mat<2, 2, float>>,
      ArrayPropertyView<glm::mat<2, 2, double>>,
      ArrayPropertyView<glm::mat<3, 3, int8_t>>,
      ArrayPropertyView<glm::mat<3, 3, uint8_t>>,
      ArrayPropertyView<glm::mat<3, 3, int16_t>>,
      ArrayPropertyView<glm::mat<3, 3, uint16_t>>,
      ArrayPropertyView<glm::mat<3, 3, int32_t>>,
      ArrayPropertyView<glm::mat<3, 3, uint32_t>>,
      ArrayPropertyView<glm::mat<3, 3, int64_t>>,
      ArrayPropertyView<glm::mat<3, 3, uint64_t>>,
      ArrayPropertyView<glm::mat<3, 3, float>>,
      ArrayPropertyView<glm::mat<3, 3, double>>,
      ArrayPropertyView<glm::mat<4, 4, int8_t>>,
      ArrayPropertyView<glm::mat<4, 4, uint8_t>>,
      ArrayPropertyView<glm::mat<4, 4, int16_t>>,
      ArrayPropertyView<glm::mat<4, 4, uint16_t>>,
      ArrayPropertyView<glm::mat<4, 4, int32_t>>,
      ArrayPropertyView<glm::mat<4, 4, uint32_t>>,
      ArrayPropertyView<glm::mat<4, 4, int64_t>>,
      ArrayPropertyView<glm::mat<4, 4, uint64_t>>,
      ArrayPropertyView<glm::mat<4, 4, float>>,
      ArrayPropertyView<glm::mat<4, 4, double>>>;
#pragma endregion

public:
  /**
   * Construct an invalid property with an unknown type.
   */
  FCesiumPropertyTableProperty()
      : _status(ECesiumPropertyTablePropertyStatus::ErrorInvalidProperty),
        _property(),
        _valueType(),
        _count(0),
        _normalized(false) {}

  /**
   * Construct a wrapper for the property table property view.
   *
   * @param value The PropertyTablePropertyView to be stored in this struct.
   */
  template <typename T>
  FCesiumPropertyTableProperty(
      const CesiumGltf::PropertyTablePropertyView<T>& value)
      : _status(ECesiumPropertyTablePropertyStatus::ErrorInvalidProperty),
        _property(value),
        _valueType(),
        _count(0),
        _normalized(false) {
    switch (value.status()) {
    case CesiumGltf::PropertyTablePropertyViewStatus::Valid:
      _status = ECesiumPropertyTablePropertyStatus::Valid;
      break;
    case CesiumGltf::PropertyTablePropertyViewStatus::ErrorInvalidPropertyTable:
    case CesiumGltf::PropertyTablePropertyViewStatus::ErrorNonexistentProperty:
    case CesiumGltf::PropertyTablePropertyViewStatus::ErrorTypeMismatch:
    case CesiumGltf::PropertyTablePropertyViewStatus::
        ErrorComponentTypeMismatch:
    case CesiumGltf::PropertyTablePropertyViewStatus::ErrorArrayTypeMismatch:
      // The status was already set in the initializer list.
      return;
    default:
      _status = ECesiumPropertyTablePropertyStatus::ErrorInvalidPropertyData;
      return;
    }

    _valueType = GetCesiumMetadataValueType<T>();
    _count = value.getArrayCount();
    _normalized = value.isNormalized();
  }

private:
  template <typename T, typename... VariantType>
  static bool
  holdsPropertyAlternative(const std::variant<VariantType...>& variant) {
    return std::holds_alternative<CesiumGltf::PropertyTablePropertyView<T>>(
        variant);
  }

  ECesiumPropertyTablePropertyStatus _status;
  PropertyType _property;
  FCesiumMetadataValueType _valueType;
  int64 _count;
  bool _normalized;

  friend class UCesiumPropertyTablePropertyBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumPropertyTablePropertyBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Gets the status of the property table property. If this property table
   * property is invalid in any way, this will briefly indicate why.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTableProperty")
  static ECesiumPropertyTablePropertyStatus GetPropertyTablePropertyStatus(
      UPARAM(ref) const FCesiumPropertyTableProperty& Property);

  /**
   * Gets the best-fitting type for the property that is accessible from
   * Blueprints. For the most precise representation of the values possible in
   * Blueprints, you should retrieve it using this type.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTableProperty")
  static ECesiumMetadataBlueprintType
  GetBlueprintType(UPARAM(ref) const FCesiumPropertyTableProperty& Property);

  /**
   * Gets the best-fitting Blueprints type for the elements in this property's
   * array values. If the given property does not contain array values, this
   * returns None.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTableProperty")
  static ECesiumMetadataBlueprintType GetArrayElementBlueprintType(
      UPARAM(ref) const FCesiumPropertyTableProperty& Property);

  /**
   * Gets the type of the metadata value as defined in the
   * EXT_structural_metadata extension. Many of these types are not accessible
   * from Blueprints, but can be converted to a Blueprint-accessible type.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTableProperty")
  static FCesiumMetadataValueType
  GetValueType(UPARAM(ref) const FCesiumPropertyTableProperty& Property);

  /**
   * Gets the number of values in the property.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTableProperty")
  static int64
  GetPropertySize(UPARAM(ref) const FCesiumPropertyTableProperty& Property);

  /**
   * Gets the number of elements in an array of this property. Only
   * applicable when the property is a fixed-length array type.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTableProperty")
  static int64 GetArraySize(UPARAM(ref)
                                const FCesiumPropertyTableProperty& Property);

  /**
   * Attempts to retrieve the value for the given feature as a boolean.
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
   * All other types return the default value. If the feature ID is
   * out-of-range, or if the property table property is somehow invalid, the
   * default value is returned.
   *
   * @param FeatureID The ID of the feature.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as a Boolean.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTableProperty")
  static bool GetBoolean(
      UPARAM(ref) const FCesiumPropertyTableProperty& Property,
      int64 FeatureID,
      bool DefaultValue = false);

  /**
   * Attempts to retrieve the value for the given feature as an unsigned
   * 8-bit integer.
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
   * In all other cases, the default value is returned. If the feature ID is
   * out-of-range, or if the property table property is somehow invalid, the
   * default value is returned.
   *
   * @param FeatureID The ID of the feature.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as a Byte.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTableProperty")
  static uint8 GetByte(
      UPARAM(ref) const FCesiumPropertyTableProperty& Property,
      int64 FeatureID,
      uint8 DefaultValue = 0);

  /**
   * Attempts to retrieve the value for the given feature as a signed 32-bit
   * integer.
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
   * In all other cases, the default value is returned. If the feature ID is
   * out-of-range, or if the property table property is somehow invalid, the
   * default value is returned.
   *
   * @param FeatureID The ID of the feature.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as an Integer.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Property")
  static int32 GetInteger(
      UPARAM(ref) const FCesiumPropertyTableProperty& Property,
      int64 FeatureID,
      int32 DefaultValue = 0);

  /**
   * Attempts to retrieve the value for the given feature as a signed 64-bit
   * integer.
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
   * In all other cases, the default value is returned. If the feature ID is
   * out-of-range, or if the property table property is somehow invalid, the
   * default value is returned.
   *
   * @param FeatureID The ID of the feature.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as an Integer64.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Property")
  static int64 GetInteger64(
      UPARAM(ref) const FCesiumPropertyTableProperty& Property,
      int64 FeatureID,
      int64 DefaultValue = 0);

  /**
   * Attempts to retrieve the value for the given feature as a single-precision
   * floating-point number.
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
   * In all other cases, the default value is returned. If the feature ID is
   * out-of-range, or if the property table property is somehow invalid, the
   * default value is returned.
   *
   * @param FeatureID The ID of the feature.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as a Float.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTableProperty")
  static float GetFloat(
      UPARAM(ref) const FCesiumPropertyTableProperty& Property,
      int64 FeatureID,
      float DefaultValue = 0.0f);

  /**
   * Attempts to retrieve the value for the given feature as a double-precision
   * floating-point number.
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
   * In all other cases, the default value is returned. If the feature ID is
   * out-of-range, or if the property table property is somehow invalid, the
   * default value is returned.
   *
   * @param FeatureID The ID of the feature.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as a Float64.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTableProperty")
  static double GetFloat64(
      UPARAM(ref) const FCesiumPropertyTableProperty& Property,
      int64 FeatureID,
      double DefaultValue = 0.0);

  /**
   * Attempts to retrieve the value for the given feature as a FIntPoint.
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
   * If the feature ID is out-of-range, or if the property table property is
   * somehow invalid, the default value is returned.
   *
   * @param FeatureID The ID of the feature.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as a FIntPoint.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTableProperty")
  static FIntPoint GetIntPoint(
      UPARAM(ref) const FCesiumPropertyTableProperty& Property,
      int64 FeatureID,
      const FIntPoint& DefaultValue);

  /**
   * Attempts to retrieve the value for the given feature as a FVector2D.
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
   * In all other cases, the default value is returned. If the feature ID is
   * out-of-range, or if the property table property is somehow invalid, the
   * default value is returned.
   *
   * @param FeatureID The ID of the feature.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as a FVector2D.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTableProperty")
  static FVector2D GetVector2D(
      UPARAM(ref) const FCesiumPropertyTableProperty& Property,
      int64 FeatureID,
      const FVector2D& DefaultValue);

  /**
   * Retrieves the value of the property for the feature with the given ID and
   * attempts to convert it to a string value.
   *
   * Numeric properties are converted to a string with `FString::Format`,
   which
   * uses the current locale.
   *
   * Boolean properties are converted to "true" or "false".
   *
   * Array properties return the `defaultValue`.
   *
   * String properties are returned directly.
   *
   * @param featureID The ID of the feature.
   * @param defaultValue The default value to use if the feature ID is invalid
   * or the feature's value cannot be converted.
   * @return The property value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Property")
  static FString GetString(
      UPARAM(ref) const FCesiumPropertyTableProperty& Property,
      int64 FeatureID,
      const FString& DefaultValue = "");

  /**
   * Retrieves the value of the property for the feature with the given ID.
   * If the property is not an array type, this method returns an empty array.
   *
   * If the property is not an array, the default value is returned.
   *
   * @param featureID The ID of the feature.
   * @return The property value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Property")
  static FCesiumPropertyArray GetArray(
      UPARAM(ref) const FCesiumPropertyTableProperty& Property,
      int64 FeatureID);

  /**
   * Retrieves the value of the property for the feature with the given ID.
   * The value is returned in a generic form that can be queried as a specific
   * type later.
   *
   * @param featureID The ID of the feature.
   * @return The property value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Property")
  static FCesiumMetadataValue GetValue(
      UPARAM(ref) const FCesiumPropertyTableProperty& Property,
      int64 FeatureID);

  /**
   * Whether this property is supposed to be normalized. Only applicable when
   * the type (or element type if this is an array) is an integer.
   *
   * @return Whether this property is normalized.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Property")
  static bool IsNormalized(UPARAM(ref)
                               const FCesiumPropertyTableProperty& Property);
};
