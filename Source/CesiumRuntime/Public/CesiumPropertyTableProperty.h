// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/PropertyTablePropertyView.h"
#include "CesiumGltf/PropertyTypeTraits.h"
#include "CesiumMetadataArray.h"
#include "CesiumMetadataValue.h"
#include "CesiumMetadataValueType.h"
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
 * A Blueprint-accessible wrapper for a metadata property in a glTF property
 * table. A property has a specific type, such as int64 or string, and a value
 * of that type for each feature in the mesh.
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
   * Construct an invalid property with unknown type.
   */
  FCesiumPropertyTableProperty()
      : _status(ECesiumPropertyTablePropertyStatus::ErrorInvalidProperty),
        _property(),
        _valueType(
            ECesiumMetadataType::Invalid,
            ECesiumMetadataComponentType::None,
            false),
        _count(0),
        _normalized(false) {}

  /**
   * Construct a wrapper for the property table property view.
   *
   * @param value The property table property to be stored in this struct.
   */
  template <typename T>
  FCesiumPropertyTableProperty(
      const CesiumGltf::PropertyTablePropertyView<T>& value)
      : _status(ECesiumPropertyTablePropertyStatus::ErrorInvalidProperty),
        _property(value),
        _valueType(
            ECesiumMetadataType::Invalid,
            ECesiumMetadataComponentType::None,
            false),
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

    ECesiumMetadataType type;
    ECesiumMetadataComponentType componentType;
    bool isArray;
    if constexpr (CesiumGltf::IsMetadataArray<T>::value) {
      using ArrayType = CesiumGltf::MetadataArrayType<T>::type;
      type =
          ECesiumMetadataType(CesiumGltf::TypeToPropertyType<ArrayType>::value);
      componentType = ECesiumMetadataComponentType(
          CesiumGltf::TypeToPropertyType<ArrayType>::component);
      isArray = true;
    } else {
      type = ECesiumMetadataType(CesiumGltf::TypeToPropertyType<T>::value);
      componentType = ECesiumMetadataComponentType(
          CesiumGltf::TypeToPropertyType<T>::component);
      isArray = false;
    }

    _valueType = {type, componentType, isArray};
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
   * Gets best-fitting type for the property table property that is accessible
   * from Blueprints. For the most precise representation of the value possible
   * in Blueprints, you should retrieve it using this type.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Property")
  static ECesiumMetadataBlueprintType
  GetBlueprintType(UPARAM(ref) const FCesiumPropertyTableProperty& Property);

  /**
   * Gets best-fitting Blueprints type for the elements in this array property.
   * If this property table property does not hold array values, this returns
   * None.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Property")
  static ECesiumMetadataBlueprintType GetArrayElementBlueprintType(
      UPARAM(ref) const FCesiumPropertyTableProperty& Property);

  /**
   * Gets the true metadata value type of the property. Many of these types are
   * not accessible from Blueprints, but can be converted to a
   * Blueprint-accessible type.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Property")
  static FCesiumMetadataValueType
  GetValueType(UPARAM(ref) const FCesiumPropertyTableProperty& Property);

  /**
   * Gets the number of metadata entries in the property.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Property")
  static int64
  GetPropertySize(UPARAM(ref) const FCesiumPropertyTableProperty& Property);

  /**
   * Gets the number of elements in an array of this property. Only
   * applicable when the property is a fixed-length array type.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Property")
  static int64
  GetPropertyArraySize(UPARAM(ref)
                           const FCesiumPropertyTableProperty& Property);

  /**
   * Retrieves the value of the property for the feature with the given ID and
   * attempts to convert it to a Boolean value.
   *
   * If the property is boolean, it is returned directly.
   *
   * If the property is numeric, zero is converted to false, while any other
   * value is converted to true.
   *
   * If the property is a string, "0", "false", and "no" (case-insensitive)
   are
   * converted to false, while "1", "true", and "yes" are converted to true.
   * All other strings, including strings that can be converted to numbers,
   * will return the default value.
   *
   * Other types of properties will return the default value.
   *
   * @param FeatureID The ID of the feature.
   * @param DefaultValue The default value to use if the feature ID is invalid
   * or the feature's value cannot be converted.
   * @return The property value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Property")
  static bool GetBoolean(
      UPARAM(ref) const FCesiumPropertyTableProperty& Property,
      int64 FeatureID,
      bool DefaultValue = false);

  /**
   * Retrieves the value of the property for the feature with the given ID and
   * attempts to convert it to an unsigned 8-bit integer value.
   *
   * If the property is an integer and between 0 and 255, it is returned
   * directly.
   *
   * If the property is a floating-point number, it is truncated (rounded
   * toward zero).
   *
   * If the property is a boolean, 0 is returned for false and 1 for true.
   *
   * If the property is a string and the entire string can be parsed as an
   * integer between 0 and 255, the parsed value is returned. The string is
   * parsed in a locale-independent way and does not support use of a comma or
   * other character to group digits.
   *
   * Otherwise, the default value is returned.
   *
   * @param FeatureID The ID of the feature.
   * @param DefaultValue The default value to use if the feature ID is invalid
   * or the feature's value cannot be converted.
   * @return The property value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Property")
  static uint8 GetByte(
      UPARAM(ref) const FCesiumPropertyTableProperty& Property,
      int64 FeatureID,
      uint8 DefaultValue = 0);

  /**
   * Retrieves the value of the property for the feature with the given ID and
   * attempts to convert it to a signed 32-bit integer value.
   *
   * If the property is an integer and between -2,147,483,647 and
   2,147,483,647,
   * it is returned directly.
   *
   * If the property is a floating-point number, it is truncated (rounded
   * toward zero).
   *
   * If the property is a boolean, 0 is returned for false and 1 for true.
   *
   * If the property is a string and the entire string can be parsed as an
   * integer in the valid range, the parsed value is returned. If it can be
   * parsed as a floating-point number, the parsed value is truncated (rounded
   * toward zero). In either case, the string is parsed in a
   locale-independent
   * way and does not support use of a comma or other character to group
   digits.
   *
   * Otherwise, the default value is returned.
   *
   * @param FeatureID The ID of the feature.
   * @param DefaultValue The default value to use if the feature ID is invalid
   * or the feature's value cannot be converted.
   * @return The property value.
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
   * Retrieves the value of the property for the feature with the given ID and
   * attempts to convert it to a signed 64-bit integer value.
   *
   * If the property is an integer and between -2^63-1 and 2^63-1, it
   * is returned directly.
   *
   * If the property is a floating-point number, it is truncated (rounded
   * toward zero).
   *
   * If the property is a boolean, 0 is returned for false and 1 for true.
   *
   * If the property is a string and the entire string can be parsed as an
   * integer in the valid range, the parsed value is returned. If it can be
   * parsed as a floating-point number, the parsed value is truncated (rounded
   * toward zero). In either case, the string is parsed in a
   locale-independent
   * way and does not support use of a comma or other character to group
   digits.
   *
   * Otherwise, the default value is returned.
   *
   * @param FeatureID The ID of the feature.
   * @param DefaultValue The default value to use if the feature ID is invalid
   * or the feature's value cannot be converted.
   * @return The property value.
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
   * Retrieves the value of the property for the feature with the given ID and
   * attempts to convert it to a 32-bit floating-point value.
   *
   * If the property is a singe-precision floating-point number, is is
   returned.
   *
   * If the property is an integer or double-precision floating-point number,
   * it is converted to the closest representable single-precision
   * floating-point number.
   *
   * If the property is a boolean, 0.0 is returned for false and 1.0 for true.
   *
   * If the property is a string and the entire string can be parsed as a
   * number, the parsed value is returned. The string is parsed in a
   * locale-independent way and does not support use of a comma or other
   * character to group digits.
   *
   * Otherwise, the default value is returned.
   *
   * @param FeatureID The ID of the feature.
   * @param DefaultValue The default value to use if the feature ID is invalid
   * or the feature's value cannot be converted.
   * @return The property value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Property")
  static float GetFloat(
      UPARAM(ref) const FCesiumPropertyTableProperty& Property,
      int64 FeatureID,
      float DefaultValue = 0.0f);

  /**
   * Retrieves the value of the property for the feature with the given ID and
   * attempts to convert it to a 64-bit floating-point value.
   *
   * If the property is a single- or double-precision floating-point number,
   is
   * is returned.
   *
   * If the property is an integer, it is converted to the closest
   representable
   * single-precision floating-point number.
   *
   * If the property is a boolean, 0.0 is returned for false and 1.0 for true.
   *
   * If the property is a string and the entire string can be parsed as a
   * number, the parsed value is returned. The string is parsed in a
   * locale-independent way and does not support use of a comma or other
   * character to group digits.
   *
   * Otherwise, the default value is returned.
   *
   * @param FeatureID The ID of the feature.
   * @param DefaultValue The default value to use if the feature ID is invalid
   * or the feature's value cannot be converted.
   * @return The property value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Property")
  static double GetFloat64(
      UPARAM(ref) const FCesiumPropertyTableProperty& Property,
      int64 FeatureID,
      double DefaultValue = 0.0);

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
  static FCesiumMetadataArray GetArray(
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
