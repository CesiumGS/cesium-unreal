// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/MetadataPropertyView.h"
#include "CesiumMetadataArray.h"
#include "CesiumMetadataGenericValue.h"
#include "CesiumMetadataValueType.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ObjectMacros.h"
#include <string_view>
#include <variant>
#include "CesiumMetadataProperty.generated.h"

/**
 * A Blueprint-accessible wrapper for a glTF metadata property. A property has a
 * particular type, such as int64 or string, and a value of that type for each
 * feature in the mesh.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumMetadataProperty {
  GENERATED_USTRUCT_BODY()

private:
  using PropertyType = std::variant<
      CesiumGltf::MetadataPropertyView<int8_t>,
      CesiumGltf::MetadataPropertyView<uint8_t>,
      CesiumGltf::MetadataPropertyView<int16_t>,
      CesiumGltf::MetadataPropertyView<uint16_t>,
      CesiumGltf::MetadataPropertyView<int32_t>,
      CesiumGltf::MetadataPropertyView<uint32_t>,
      CesiumGltf::MetadataPropertyView<int64_t>,
      CesiumGltf::MetadataPropertyView<uint64_t>,
      CesiumGltf::MetadataPropertyView<float>,
      CesiumGltf::MetadataPropertyView<double>,
      CesiumGltf::MetadataPropertyView<bool>,
      CesiumGltf::MetadataPropertyView<std::string_view>,
      CesiumGltf::MetadataPropertyView<CesiumGltf::MetadataArrayView<int8_t>>,
      CesiumGltf::MetadataPropertyView<CesiumGltf::MetadataArrayView<uint8_t>>,
      CesiumGltf::MetadataPropertyView<CesiumGltf::MetadataArrayView<int16_t>>,
      CesiumGltf::MetadataPropertyView<CesiumGltf::MetadataArrayView<uint16_t>>,
      CesiumGltf::MetadataPropertyView<CesiumGltf::MetadataArrayView<int32_t>>,
      CesiumGltf::MetadataPropertyView<CesiumGltf::MetadataArrayView<uint32_t>>,
      CesiumGltf::MetadataPropertyView<CesiumGltf::MetadataArrayView<int64_t>>,
      CesiumGltf::MetadataPropertyView<CesiumGltf::MetadataArrayView<uint64_t>>,
      CesiumGltf::MetadataPropertyView<CesiumGltf::MetadataArrayView<float>>,
      CesiumGltf::MetadataPropertyView<CesiumGltf::MetadataArrayView<double>>,
      CesiumGltf::MetadataPropertyView<CesiumGltf::MetadataArrayView<bool>>,
      CesiumGltf::MetadataPropertyView<
          CesiumGltf::MetadataArrayView<std::string_view>>>;

public:
  /**
   * Construct an empty property with unknown type.
   */
  FCesiumMetadataProperty()
      : _property(), _type(ECesiumMetadataValueType::None) {}

  /**
   * Construct a wrapper for the property view.
   *
   * @param value The property to be stored in this struct
   */
  template <typename T>
  FCesiumMetadataProperty(const CesiumGltf::MetadataPropertyView<T>& value)
      : _property(value), _type(ECesiumMetadataValueType::None) {
    if (holdsPropertyAlternative<bool>(_property)) {
      _type = ECesiumMetadataValueType::Boolean;
    } else if (holdsPropertyAlternative<uint8_t>(_property)) {
      _type = ECesiumMetadataValueType::Byte;
    } else if (
        holdsPropertyAlternative<int8_t>(_property) ||
        holdsPropertyAlternative<int16_t>(_property) ||
        holdsPropertyAlternative<uint16_t>(_property) ||
        holdsPropertyAlternative<int32_t>(_property)) {
      _type = ECesiumMetadataValueType::Integer;
    } else if (
        holdsPropertyAlternative<uint32_t>(_property) ||
        holdsPropertyAlternative<int64_t>(_property)) {
      _type = ECesiumMetadataValueType::Integer64;
    } else if (holdsPropertyAlternative<uint64_t>(_property)) {
      _type = ECesiumMetadataValueType::String;
    } else if (holdsPropertyAlternative<float>(_property)) {
      _type = ECesiumMetadataValueType::Float;
    } else if (holdsPropertyAlternative<double>(_property)) {
      _type = ECesiumMetadataValueType::String;
    } else if (holdsPropertyAlternative<std::string_view>(_property)) {
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
   * Queries the number of features in the property.
   */
  size_t GetNumberOfFeatures() const;

  /**
   * Retrieves the value of the property for the feature with the given ID and
   * attempts to convert it to a Boolean value.
   *
   * If the property is boolean, it is returned directly.
   *
   * If the property is numeric, zero is converted to false, while any other
   * value is converted to true.
   *
   * If the property is a string, "0", "false", and "no" (case-insensitive) are
   * converted to false, while "1", "true", and "yes" are converted to true.
   * All other strings, including strings that can be converted to numbers,
   * will return the default value.
   *
   * Other types of properties will return the default value.
   *
   * @param featureID The ID of the feature.
   * @param defaultValue The default value to use if the feature ID is invalid
   * or the feature's value cannot be converted.
   * @return The property value.
   */
  bool GetBoolean(int64 featureID, bool defaultValue) const;

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
   * @param featureID The ID of the feature.
   * @param defaultValue The default value to use if the feature ID is invalid
   * or the feature's value cannot be converted.
   * @return The property value.
   */
  uint8 GetByte(int64 featureID, uint8 defaultValue) const;

  /**
   * Retrieves the value of the property for the feature with the given ID and
   * attempts to convert it to a signed 32-bit integer value.
   *
   * If the property is an integer and between -2,147,483,647 and 2,147,483,647,
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
   * toward zero). In either case, the string is parsed in a locale-independent
   * way and does not support use of a comma or other character to group digits.
   *
   * Otherwise, the default value is returned.
   *
   * @param featureID The ID of the feature.
   * @param defaultValue The default value to use if the feature ID is invalid
   * or the feature's value cannot be converted.
   * @return The property value.
   */
  int32 GetInteger(int64 featureID, int32 defaultValue) const;

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
   * toward zero). In either case, the string is parsed in a locale-independent
   * way and does not support use of a comma or other character to group digits.
   *
   * Otherwise, the default value is returned.
   *
   * @param featureID The ID of the feature.
   * @param defaultValue The default value to use if the feature ID is invalid
   * or the feature's value cannot be converted.
   * @return The property value.
   */
  int64 GetInteger64(int64 featureID, int64 defaultValue) const;

  /**
   * Retrieves the value of the property for the feature with the given ID and
   * attempts to convert it to a 32-bit floating-point value.
   *
   * If the property is a singe-precision floating-point number, is is returned.
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
   * @param featureID The ID of the feature.
   * @param defaultValue The default value to use if the feature ID is invalid
   * or the feature's value cannot be converted.
   * @return The property value.
   */
  float GetFloat(int64 featureID, float defaultValue) const;

  /**
   * Retrieves the value of the property for the feature with the given ID and
   * attempts to convert it to a string value.
   *
   * Numeric properties are converted to a string with `FString::Format`, which
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
  FString GetString(int64 featureID, const FString& defaultValue) const;

  /**
   * Retrieves the value of the property for the feature with the given ID.
   * If the property is not an array type, this method returns an empty array.
   *
   * If the property is not an array, the default value is returned.
   *
   * @param featureID The ID of the feature.
   * @return The property value.
   */
  FCesiumMetadataArray GetArray(int64 featureID) const;

  /**
   * Retrieves the value of the property for the feature with the given ID.
   * The value is returned in a generic form that can be queried as a specific
   * type later.
   *
   * @param featureID The ID of the feature.
   * @return The property value.
   */
  FCesiumMetadataGenericValue GetGenericValue(int64 featureID) const;

private:
  template <typename T, typename... VariantType>
  static bool
  holdsPropertyAlternative(const std::variant<VariantType...>& variant) {
    return std::holds_alternative<CesiumGltf::MetadataPropertyView<T>>(variant);
  }

  PropertyType _property;
  ECesiumMetadataValueType _type;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumMetadataPropertyBlueprintLibrary
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
      Category = "Cesium|Metadata|Property")
  static ECesiumMetadataValueType
  GetType(UPARAM(ref) const FCesiumMetadataProperty& Property);

  /**
   * Queries the number of features in the property.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|Property")
  static int64 GetNumberOfFeatures(UPARAM(ref)
                                       const FCesiumMetadataProperty& Property);

  /**
   * Retrieves the value of the property for the feature with the given ID and
   * attempts to convert it to a Boolean value.
   *
   * If the property is boolean, it is returned directly.
   *
   * If the property is numeric, zero is converted to false, while any other
   * value is converted to true.
   *
   * If the property is a string, "0", "false", and "no" (case-insensitive) are
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
      UPARAM(ref) const FCesiumMetadataProperty& Property,
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
      UPARAM(ref) const FCesiumMetadataProperty& Property,
      int64 FeatureID,
      uint8 DefaultValue = 0);

  /**
   * Retrieves the value of the property for the feature with the given ID and
   * attempts to convert it to a signed 32-bit integer value.
   *
   * If the property is an integer and between -2,147,483,647 and 2,147,483,647,
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
   * toward zero). In either case, the string is parsed in a locale-independent
   * way and does not support use of a comma or other character to group digits.
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
      UPARAM(ref) const FCesiumMetadataProperty& Property,
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
   * toward zero). In either case, the string is parsed in a locale-independent
   * way and does not support use of a comma or other character to group digits.
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
      UPARAM(ref) const FCesiumMetadataProperty& Property,
      int64 FeatureID,
      int64 DefaultValue = 0);

  /**
   * Retrieves the value of the property for the feature with the given ID and
   * attempts to convert it to a 32-bit floating-point value.
   *
   * If the property is a singe-precision floating-point number, is is returned.
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
      UPARAM(ref) const FCesiumMetadataProperty& Property,
      int64 FeatureID,
      float DefaultValue = 0.0f);

  /**
   * Retrieves the value of the property for the feature with the given ID and
   * attempts to convert it to a string value.
   *
   * Numeric properties are converted to a string with `FString::Format`, which
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
      UPARAM(ref) const FCesiumMetadataProperty& Property,
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
      UPARAM(ref) const FCesiumMetadataProperty& Property,
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
  static FCesiumMetadataGenericValue GetGenericValue(
      UPARAM(ref) const FCesiumMetadataProperty& Property,
      int64 FeatureID);
};
