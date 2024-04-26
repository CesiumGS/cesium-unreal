// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/PropertyTablePropertyView.h"
#include "CesiumGltf/PropertyTypeTraits.h"
#include "CesiumMetadataValue.h"
#include "CesiumMetadataValueType.h"
#include "CesiumPropertyArray.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ObjectMacros.h"
#include <any>
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
  /* The property table property is empty but has a specified default value. */
  EmptyPropertyWithDefault,
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
public:
  /**
   * Construct an invalid property with an unknown type.
   */
  FCesiumPropertyTableProperty()
      : _status(ECesiumPropertyTablePropertyStatus::ErrorInvalidProperty),
        _property(),
        _valueType(),
        _normalized(false) {}

  /**
   * Construct a wrapper for the property table property view.
   *
   * @param value The PropertyTablePropertyView to be stored in this struct.
   */
  template <typename T, bool Normalized>
  FCesiumPropertyTableProperty(
      const CesiumGltf::PropertyTablePropertyView<T, Normalized>& Property)
      : _status(ECesiumPropertyTablePropertyStatus::ErrorInvalidProperty),
        _property(Property),
        _valueType(),
        _normalized(Normalized) {
    switch (Property.status()) {
    case CesiumGltf::PropertyTablePropertyViewStatus::Valid:
      _status = ECesiumPropertyTablePropertyStatus::Valid;
      break;
    case CesiumGltf::PropertyTablePropertyViewStatus::EmptyPropertyWithDefault:
      _status = ECesiumPropertyTablePropertyStatus::EmptyPropertyWithDefault;
      break;
    case CesiumGltf::PropertyTablePropertyViewStatus::ErrorInvalidPropertyTable:
    case CesiumGltf::PropertyTablePropertyViewStatus::ErrorNonexistentProperty:
    case CesiumGltf::PropertyTablePropertyViewStatus::ErrorTypeMismatch:
    case CesiumGltf::PropertyTablePropertyViewStatus::
        ErrorComponentTypeMismatch:
    case CesiumGltf::PropertyTablePropertyViewStatus::ErrorArrayTypeMismatch:
    case CesiumGltf::PropertyTablePropertyViewStatus::ErrorInvalidNormalization:
    case CesiumGltf::PropertyTablePropertyViewStatus::
        ErrorNormalizationMismatch:
    case CesiumGltf::PropertyTablePropertyViewStatus::ErrorInvalidOffset:
    case CesiumGltf::PropertyTablePropertyViewStatus::ErrorInvalidScale:
    case CesiumGltf::PropertyTablePropertyViewStatus::ErrorInvalidMax:
    case CesiumGltf::PropertyTablePropertyViewStatus::ErrorInvalidMin:
    case CesiumGltf::PropertyTablePropertyViewStatus::ErrorInvalidNoDataValue:
    case CesiumGltf::PropertyTablePropertyViewStatus::ErrorInvalidDefaultValue:
      // The status was already set in the initializer list.
      return;
    default:
      _status = ECesiumPropertyTablePropertyStatus::ErrorInvalidPropertyData;
      return;
    }

    _valueType = TypeToMetadataValueType<T>();
    _normalized = Normalized;
  }

private:
  ECesiumPropertyTablePropertyStatus _status;

  std::any _property;

  FCesiumMetadataValueType _valueType;
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

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  /**
   * Gets the best-fitting Blueprints type for the elements in this property's
   * array values. If the given property does not contain array values, this
   * returns None.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Meta =
          (DeprecatedFunction,
           DeprecationMessage = "Use GetArrayElementBlueprintType instead."))
  static ECesiumMetadataBlueprintType
  GetBlueprintComponentType(UPARAM(ref)
                                const FCesiumPropertyTableProperty& Property);
  PRAGMA_ENABLE_DEPRECATION_WARNINGS

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
  GetTrueType(UPARAM(ref) const FCesiumPropertyTableProperty& Value);

  /**
   * Gets true type of the elements in this array property. If this is not an
   * array property, the component type will be None. Many of these types are
   * not accessible from Blueprints, but can be converted to a
   * Blueprint-accessible type.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "CesiumMetadataTrueType is deprecated. Use GetValueType to get the CesiumMetadataValueType instead."))
  static ECesiumMetadataTrueType_DEPRECATED
  GetTrueComponentType(UPARAM(ref) const FCesiumPropertyTableProperty& Value);

  PRAGMA_ENABLE_DEPRECATION_WARNINGS

  /**
   * Gets the number of values in the property.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTableProperty")
  static int64
  GetPropertySize(UPARAM(ref) const FCesiumPropertyTableProperty& Property);

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  /**
   * Gets the number of values in this property.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Meta =
          (DeprecatedFunction,
           DeprecationMessage = "Use GetPropertySize instead."))
  static int64
  GetNumberOfFeatures(UPARAM(ref) const FCesiumPropertyTableProperty& Property);
  PRAGMA_ENABLE_DEPRECATION_WARNINGS

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

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  /**
   * Gets the number of elements in an array of this property. Only
   * applicable when the property is a fixed-length array type.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Meta =
          (DeprecatedFunction,
           DeprecationMessage = "Use GetArraySize instead."))
  static int64
  GetComponentCount(UPARAM(ref) const FCesiumPropertyTableProperty& Property);
  PRAGMA_ENABLE_DEPRECATION_WARNINGS

  /**
   * Attempts to retrieve the value for the given feature as a boolean.
   *
   * For numeric properties, the raw value for a given feature will be
   * transformed by the property's normalization, scale, and offset before it is
   * further converted. If the raw value is equal to the property's "no data"
   * value, then the property's default value will be converted if possible. If
   * the property-defined default value cannot be converted, or does not exist,
   * then the user-defined default value is returned.
   *
   * Property values are converted as follows:
   *
   * - If the value is a boolean, it is returned as-is.
   *
   * - If the value is a scalar, zero is converted to false, while any other
   * value is converted to true.
   *
   * - If the value is a string, "0", "false", and "no" (case-insensitive) are
   * converted to false, while "1", "true", and "yes" are converted to true.
   * All other strings, including strings that can be converted to numbers,
   * will return the user-defined default value.
   *
   * All other types return the user-defined default value. If the feature ID is
   * out-of-range, or if the property table property is somehow invalid, the
   * user-defined default value is returned.
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
   * For numeric properties, the raw value for a given feature will be
   * transformed by the property's normalization, scale, and offset before it is
   * further converted. If the raw value is equal to the property's "no data"
   * value, then the property's default value will be converted if possible. If
   * the property-defined default value cannot be converted, or does not exist,
   * then the user-defined default value is returned.
   *
   * Property values are converted as follows:
   *
   * - If the value is an integer between 0 and 255, it is returned as-is.
   * Otherwise, if the value is a floating-point number in the aforementioned
   * range, it is truncated (rounded toward zero) and returned.
   *
   * - If the value is a boolean, 1 is returned for true and 0 for false.
   *
   * - If the value is a string and the entire string can be parsed as an
   * integer between 0 and 255, the parsed value is returned. The string is
   * parsed in a locale-independent way and does not support the use of commas
   * or other delimiters to group digits together.
   *
   * In all other cases, the user-defined default value is returned. If the
   * feature ID is out-of-range, or if the property table property is somehow
   * invalid, the user-defined default value is returned.
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
   * For numeric properties, the raw value for a given feature will be
   * transformed by the property's normalization, scale, and offset before it is
   * further converted. If the raw value is equal to the property's "no data"
   * value, then the property's default value will be converted if possible. If
   * the property-defined default value cannot be converted, or does not exist,
   * then the user-defined default value is returned.
   *
   * Property values are converted as follows:
   *
   * - If the value is an integer between -2,147,483,648 and 2,147,483,647, it
   * is returned as-is. Otherwise, if the value is a floating-point number in
   * the aforementioned range, it is truncated (rounded toward zero) and
   * returned.
   *
   * - If the value is a boolean, 1 is returned for true and 0 for false.
   *
   * - If the value is a string and the entire string can be parsed as an
   * integer in the valid range, the parsed value is returned. If it can be
   * parsed as a floating-point number, the parsed value is truncated (rounded
   * toward zero). In either case, the string is parsed in a locale-independent
   * way and does not support the use of commas or other delimiters to group
   * digits together.
   *
   * In all other cases, the user-defined default value is returned. If the
   * feature ID is out-of-range, or if the property table property is somehow
   * invalid, the user-defined default value is returned.
   *
   * @param FeatureID The ID of the feature.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as an Integer.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTableProperty")
  static int32 GetInteger(
      UPARAM(ref) const FCesiumPropertyTableProperty& Property,
      int64 FeatureID,
      int32 DefaultValue = 0);

  /**
   * Attempts to retrieve the value for the given feature as a signed 64-bit
   * integer.
   *
   * For numeric properties, the raw value for a given feature will be
   * transformed by the property's normalization, scale, and offset before it is
   * further converted. If the raw value is equal to the property's "no data"
   * value, then the property's default value will be converted if possible. If
   * the property-defined default value cannot be converted, or does not exist,
   * then the user-defined default value is returned.
   *
   * Property values are converted as follows:
   *
   * - If the value is an integer and between -2^63 and (2^63 - 1), it is
   * returned as-is. Otherwise, if the value is a floating-point number in the
   * aforementioned range, it is truncated (rounded toward zero) and returned.
   *
   * - If the value is a boolean, 1 is returned for true and 0 for false.
   *
   * - If the value is a string and the entire string can be parsed as an
   * integer in the valid range, the parsed value is returned. If it can be
   * parsed as a floating-point number, the parsed value is truncated (rounded
   * toward zero). In either case, the string is parsed in a locale-independent
   * way and does not support the use of commas or other delimiters to group
   * digits together.
   *
   * In all other cases, the user-defined default value is returned. If the
   * feature ID is out-of-range, or if the property table property is somehow
   * invalid, the user-defined default value is returned.
   *
   * @param FeatureID The ID of the feature.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as an Integer64.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTableProperty")
  static int64 GetInteger64(
      UPARAM(ref) const FCesiumPropertyTableProperty& Property,
      int64 FeatureID,
      int64 DefaultValue = 0);

  /**
   * Attempts to retrieve the value for the given feature as a single-precision
   * floating-point number.
   *
   * For numeric properties, the raw value for a given feature will be
   * transformed by the property's normalization, scale, and offset before it is
   * further converted. If the raw value is equal to the property's "no data"
   * value, then the property's default value will be converted if possible. If
   * the property-defined default value cannot be converted, or does not exist,
   * then the user-defined default value is returned.
   *
   * Property values are converted as follows:
   *
   * - If the value is already a single-precision floating-point
   * number, it is returned as-is.
   *
   * - If the value is a scalar of any other type within the range of values
   * that a single-precision float can represent, it is converted to its closest
   * representation as a single-precision float and returned.
   *
   * - If the value is a boolean, 1.0f is returned for true and 0.0f for false.
   *
   * - If the value is a string, and the entire string can be parsed as a
   * number, the parsed value is returned. The string is parsed in a
   * locale-independent way and does not support the use of a comma or other
   * delimiter to group digits together.
   *
   * In all other cases, the user-defined default value is returned. If the
   * feature ID is out-of-range, or if the property table property is somehow
   * invalid, the user-defined default value is returned.
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
   * For numeric properties, the raw value for a given feature will be
   * transformed by the property's normalization, scale, and offset before it is
   * further converted. If the raw value is equal to the property's "no data"
   * value, then the property's default value will be converted if possible. If
   * the property-defined default value cannot be converted, or does not exist,
   * then the user-defined default value is returned.
   *
   * Property values are converted as follows:
   *
   * - If the value is a single- or double-precision floating-point number, it
   * is returned as-is.
   *
   * - If the value is an integer, it is converted to the closest representable
   * double-precision floating-point number.
   *
   * - If the value is a boolean, 1.0 is returned for true and 0.0 for false.
   *
   * - If the value is a string and the entire string can be parsed as a
   * number, the parsed value is returned. The string is parsed in a
   * locale-independent way and does not support the use of commas or other
   * delimiters to group digits together.
   *
   * In all other cases, the user-defined default value is returned. If the
   * feature ID is out-of-range, or if the property table property is somehow
   * invalid, the user-defined default value is returned.
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
   * For numeric properties, the raw value for a given feature will be
   * transformed by the property's normalization, scale, and offset before it is
   * further converted. If the raw value is equal to the property's "no data"
   * value, then the property's default value will be converted if possible. If
   * the property-defined default value cannot be converted, or does not exist,
   * then the user-defined default value is returned.
   *
   * Property values are converted as follows:
   *
   * - If the value is a 2-dimensional vector, its components will be converted
   * to 32-bit signed integers if possible.
   *
   * - If the value is a 3- or 4-dimensional vector, it will use the first two
   * components to construct the FIntPoint.
   *
   * - If the value is a scalar that can be converted to a 32-bit signed
   * integer, the resulting FIntPoint will have this value in both of its
   * components.
   *
   * - If the value is a boolean, (1, 1) is returned for true, while (0, 0) is
   * returned for false.
   *
   * - If the value is a string that can be parsed as a FIntPoint, the parsed
   * value is returned. The string must be formatted as "X=... Y=...".
   *
   * In all other cases, the user-defined default value is returned. In all
   * vector cases, if any of the relevant components cannot be represented as a
   * 32-bit signed, the default value is returned.
   *
   * If the feature ID is out-of-range, or if the property table property is
   * somehow invalid, the user-defined default value is returned.
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
   * For numeric properties, the raw value for a given feature will be
   * transformed by the property's normalization, scale, and offset before it is
   * further converted. If the raw value is equal to the property's "no data"
   * value, then the property's default value will be converted if possible. If
   * the property-defined default value cannot be converted, or does not exist,
   * then the user-defined default value is returned.
   *
   * Property values are converted as follows:
   *
   * - If the value is a 2-dimensional vector, its components will be converted
   * to double-precision floating-point numbers.
   *
   * - If the value is a 3- or 4-dimensional vector, it will use the first two
   * components to construct the FVector2D.
   *
   * - If the value is a scalar, the resulting FVector2D will have this value in
   * both of its components.
   *
   * - If the value is a boolean, (1.0, 1.0) is returned for true, while (0.0,
   * 0.0) is returned for false.
   *
   * - If the value is a string that can be parsed as a FVector2D, the parsed
   * value is returned. The string must be formatted as "X=... Y=...".
   *
   * In all other cases, the user-defined default value is returned. If the
   * feature ID is out-of-range, or if the property table property is somehow
   * invalid, the user-defined default value is returned.
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
   * Attempts to retrieve the value for the given feature as a FIntVector.
   *
   * For numeric properties, the raw value for a given feature will be
   * transformed by the property's normalization, scale, and offset before it is
   * further converted. If the raw value is equal to the property's "no data"
   * value, then the property's default value will be converted if possible. If
   * the property-defined default value cannot be converted, or does not exist,
   * then the user-defined default value is returned.
   *
   * Property values are converted as follows:
   *
   * - If the value is a 3-dimensional vector, its components will be converted
   * to 32-bit signed integers if possible.
   *
   * - If the value is a 4-dimensional vector, it will use the first three
   * components to construct the FIntVector.
   *
   * - If the value is a 2-dimensional vector, it will become the XY-components
   * of the FIntVector. The Z component will be set to zero.
   *
   * - If the value is a scalar that can be converted to a 32-bit signed
   * integer, the resulting FIntVector will have this value in all of its
   * components.
   *
   * - If the value is a boolean, (1, 1, 1) is returned for true, while (0, 0,
   * 0) is returned for false.
   *
   * - If the value is a string that can be parsed as a FIntVector, the parsed
   * value is returned. The string must be formatted as "X=... Y=... Z=".
   *
   * In all other cases, the user-defined default value is returned. In all
   * vector cases, if any of the relevant components cannot be represented as a
   * 32-bit signed integer, the default value is returned.
   *
   * If the feature ID is out-of-range, or if the property table property is
   * somehow invalid, the user-defined default value is returned.
   *
   * @param FeatureID The ID of the feature.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as a FIntVector.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTableProperty")
  static FIntVector GetIntVector(
      UPARAM(ref) const FCesiumPropertyTableProperty& Property,
      int64 FeatureID,
      const FIntVector& DefaultValue);

  /**
   * Attempts to retrieve the value for the given feature as a FVector3f.
   *
   * For numeric properties, the raw value for a given feature will be
   * transformed by the property's normalization, scale, and offset before it is
   * further converted. If the raw value is equal to the property's "no data"
   * value, then the property's default value will be converted if possible. If
   * the property-defined default value cannot be converted, or does not exist,
   * then the user-defined default value is returned.
   *
   * Property values are converted as follows:
   *
   * - If the value is a 3-dimensional vector, its components will be converted
   * to the closest representable single-precision floats, if possible.
   *
   * - If the value is a 4-dimensional vector, a FVector3f containing the first
   * three components will be returned.
   *
   * - If the value is a 2-dimensional vector, it will become the XY-components
   * of the FVector3f. The Z-component will be set to zero.
   *
   * - If the value is a scalar that can be converted to a single-precision
   * floating-point number, then the resulting FVector3f will have this value in
   * all of its components.
   *
   * - If the value is a boolean, (1.0f, 1.0f, 1.0f) is returned for true, while
   * (0.0f, 0.0f, 0.0f) is returned for false.
   *
   * - If the value is a string that can be parsed as a FVector3f, the parsed
   * value is returned. The string must be formatted as "X=... Y=... Z=".
   *
   * In all other cases, the user-defined default value is returned. In all
   * vector cases, if any of the relevant components cannot be represented as a
   * single-precision float, the user-defined default value is returned.
   *
   * If the feature ID is out-of-range, or if the property table property is
   * somehow invalid, the user-defined default value is returned.
   *
   * @param FeatureID The ID of the feature.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as a FVector3f.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTableProperty")
  static FVector3f GetVector3f(
      UPARAM(ref) const FCesiumPropertyTableProperty& Property,
      int64 FeatureID,
      const FVector3f& DefaultValue);

  /**
   * Attempts to retrieve the value for the given feature as a FVector.
   *
   * For numeric properties, the raw value for a given feature will be
   * transformed by the property's normalization, scale, and offset before it is
   * further converted. If the raw value is equal to the property's "no data"
   * value, then the property's default value will be converted if possible. If
   * the property-defined default value cannot be converted, or does not exist,
   * then the user-defined default value is returned.
   *
   * Property values are converted as follows:
   *
   * - If the value is a 3-dimensional vector, its components will be converted
   * to double-precision floating-point numbers.
   *
   * - If the value is a 4-dimensional vector, a FVector containing the first
   * three components will be returned.
   *
   * - If the value is a 2-dimensional vector, it will become the XY-components
   * of the FVector. The Z-component will be set to zero.
   *
   * - If the value is a scalar, then the resulting FVector will have this value
   * as a double-precision floating-point number in all of its components.
   *
   * - If the value is a boolean, (1.0, 1.0, 1.0) is returned for true, while
   * (0.0, 0.0, 0.0) is returned for false.
   *
   * - If the value is a string that can be parsed as a FVector, the parsed
   * value is returned. The string must be formatted as "X=... Y=... Z=".
   *
   * In all other cases, the user-defined default value is returned. If the
   * feature ID is out-of-range, or if the property table property is somehow
   * invalid, the user-defined default value is returned.
   *
   * @param FeatureID The ID of the feature.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as a FVector.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTableProperty")
  static FVector GetVector(
      UPARAM(ref) const FCesiumPropertyTableProperty& Property,
      int64 FeatureID,
      const FVector& DefaultValue);

  /**
   * Attempts to retrieve the value for the given feature as a FVector4.
   *
   * For numeric properties, the raw value for a given feature will be
   * transformed by the property's normalization, scale, and offset before it is
   * further converted. If the raw value is equal to the property's "no data"
   * value, then the property's default value will be converted if possible. If
   * the property-defined default value cannot be converted, or does not exist,
   * then the user-defined default value is returned.
   *
   * Property values are converted as follows:
   *
   * - If the value is a 4-dimensional vector, its components will be converted
   * to double-precision floating-point numbers.
   *
   * - If the value is a 3-dimensional vector, it will become the XYZ-components
   * of the FVector4. The W-component will be set to zero.
   *
   * - If the value is a 2-dimensional vector, it will become the XY-components
   * of the FVector4. The Z- and W-components will be set to zero.
   *
   * - If the value is a scalar, then the resulting FVector4 will have this
   * value as a double-precision floating-point number in all of its components.
   *
   * - If the value is a boolean, (1.0, 1.0, 1.0, 1.0) is returned for true,
   * while (0.0, 0.0, 0.0, 0.0) is returned for false.
   *
   * - If the value is a string that can be parsed as a FVector4, the parsed
   * value is returned. This follows the rules of FVector4::InitFromString. The
   * string must be formatted as "X=... Y=... Z=... W=...". The W-component is
   * optional; if absent, it will be set to 1.0.
   *
   * In all other cases, the user-defined default value is returned. If the
   * feature ID is out-of-range, or if the property table property is somehow
   * invalid, the user-defined default value is returned.
   *
   * @param FeatureID The ID of the feature.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as a FVector4.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTableProperty")
  static FVector4 GetVector4(
      UPARAM(ref) const FCesiumPropertyTableProperty& Property,
      int64 FeatureID,
      const FVector4& DefaultValue);

  /**
   * Attempts to retrieve the value for the given feature as a FMatrix.
   *
   * For numeric properties, the raw value for a given feature will be
   * transformed by the property's normalization, scale, and offset before it is
   * further converted. If the raw value is equal to the property's "no data"
   * value, then the property's default value will be converted if possible. If
   * the property-defined default value cannot be converted, or does not exist,
   * then the user-defined default value is returned.
   *
   * Property values are converted as follows:
   *
   * - If the value is a 4-by-4 matrix, its components will be converted to
   * double-precision floating-point numbers.
   *
   * - If the value is a 3-by-3 matrix, it will initialize the corresponding
   * entries of the FMatrix, while all other entries are set to zero. In other
   * words, the 3-by-3 matrix is returned in an FMatrix where the fourth row and
   * column are filled with zeroes.
   *
   * - If the value is a 2-by-2 matrix, it will initialize the corresponding
   * entries of the FMatrix, while all other entries are set to zero. In other
   * words, the 2-by-2 matrix is returned in an FMatrix where the third and
   * fourth rows / columns are filled with zeroes.
   *
   * - If the value is a scalar, then the resulting FMatrix will have this value
   * along its diagonal, including the very last component. All other entries
   * will be zero.
   *
   * - If the value is a boolean, it is converted to 1.0 for true and 0.0 for
   * false. Then, the resulting FMatrix will have this value along its diagonal,
   * including the very last component. All other entries will be zero.
   *
   * In all other cases, the user-defined default value is returned. If the
   * feature ID is out-of-range, or if the property table property is somehow
   * invalid, the user-defined default value is returned.
   *
   * @param FeatureID The ID of the feature.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as a FMatrix.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTableProperty")
  static FMatrix GetMatrix(
      UPARAM(ref) const FCesiumPropertyTableProperty& Property,
      int64 FeatureID,
      const FMatrix& DefaultValue);

  /**
   * Attempts to retrieve the value for the given feature as a FString.
   *
   * For numeric properties, the raw value for a given feature will be
   * transformed by the property's normalization, scale, and offset before it is
   * further converted. If the raw value is equal to the property's "no data"
   * value, then the property's default value will be converted if possible. If
   * the property-defined default value cannot be converted, or does not exist,
   * then the user-defined default value is returned.
   *
   * Property values are converted as follows:
   *
   * - String properties are returned as-is.
   *
   * - Scalar values are converted to a string with `std::to_string`.
   *
   * - Boolean properties are converted to "true" or "false".
   *
   * - Vector properties are returned as strings in the format "X=... Y=...
   * Z=... W=..." depending on how many components they have.
   *
   * - Matrix properties are returned as strings row-by-row, where each row's
   * values are printed between square brackets. For example, a 2-by-2 matrix
   * will be printed out as "[A B] [C D]".
   *
   * - Array properties return the user-defined default value.
   *
   * If the feature ID is out-of-range, or if the property table property is
   * somehow invalid, the user-defined default value is returned.
   *
   * @param FeatureID The ID of the feature.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as a FString.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTableProperty")
  static FString GetString(
      UPARAM(ref) const FCesiumPropertyTableProperty& Property,
      int64 FeatureID,
      const FString& DefaultValue = "");

  /**
   * Attempts to retrieve the value for the given feature as a
   * FCesiumPropertyArray. If the property is not an array type, this returns an
   * empty array.
   *
   * For numeric array properties, the raw array value for a given feature will
   * be transformed by the property's normalization, scale, and offset before it
   * is further converted. If the raw value is equal to the property's "no data"
   * value, then the property's default value will be converted if possible. If
   * the property-defined default value cannot be converted, or does not exist,
   * then the user-defined default value is returned.
   *
   * @param featureID The ID of the feature.
   * @return The property value as a FCesiumPropertyArray.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTableProperty")
  static FCesiumPropertyArray GetArray(
      UPARAM(ref) const FCesiumPropertyTableProperty& Property,
      int64 FeatureID);

  /**
   * Retrieves the value of the property for the given feature. This allows the
   * value to be acted on more generically; its true value can be retrieved
   * later as a specific Blueprints type.
   *
   * For numeric properties, the raw value for a given feature will be
   * transformed by the property's normalization, scale, and offset before it is
   * returned. If the raw value is equal to the property's "no data" value, an
   * empty value will be returned. However, if the property itself specifies a
   * default value, then the property-defined default value will be returned.
   *
   * @param featureID The ID of the feature.
   * @return The property value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTableProperty")
  static FCesiumMetadataValue GetValue(
      UPARAM(ref) const FCesiumPropertyTableProperty& Property,
      int64 FeatureID);

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  /**
   * Retrieves the value of the property for the given feature. This allows the
   * value to be acted on more generically; its true value can be retrieved
   * later as a specific Blueprints type.
   *
   * @param featureID The ID of the feature.
   * @return The property value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Meta = (DeprecatedFunction, DeprecatedMessage = "Use GetValue instead."))
  static FCesiumMetadataValue GetGenericValue(
      UPARAM(ref) const FCesiumPropertyTableProperty& Property,
      int64 FeatureID);
  PRAGMA_ENABLE_DEPRECATION_WARNINGS

  /**
   * Retrieves the raw value of the property for the given feature. This is the
   * value of the property without normalization, offset, or scale applied.
   *
   * If this property specifies a "no data" value, and the raw value is equal to
   * this "no data" value, the value is returned as-is.
   *
   * If this property is an empty property with a specified default value, it
   * will not have any raw data to retrieve. The returned value will be empty.
   * @param featureID The ID of the feature.
   * @return The raw property value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTableProperty")
  static FCesiumMetadataValue GetRawValue(
      UPARAM(ref) const FCesiumPropertyTableProperty& Property,
      int64 FeatureID);

  /**
   * Whether this property is normalized. Only applicable when this property has
   * an integer component type.
   *
   * @return Whether this property is normalized.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTableProperty")
  static bool IsNormalized(UPARAM(ref)
                               const FCesiumPropertyTableProperty& Property);

  /**
   * Gets the offset of this property. This can be defined by the class property
   * that it implements, or overridden by the instance of the property itself.
   *
   * This is only applicable to properties with floating-point or normalized
   * integer component types. If an offset is not defined or applicable, this
   * returns an empty value.
   *
   * @return The offset of the property.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTableProperty")
  static FCesiumMetadataValue
  GetOffset(UPARAM(ref) const FCesiumPropertyTableProperty& Property);

  /**
   * Gets the scale of this property. This can be defined by the class property
   * that it implements, or overridden by the instance of the property itself.
   *
   * This is only applicable to properties with floating-point or normalized
   * integer component types. If a scale is not defined or applicable, this
   * returns an empty value.
   *
   * @return The scale of the property.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTableProperty")
  static FCesiumMetadataValue
  GetScale(UPARAM(ref) const FCesiumPropertyTableProperty& Property);

  /**
   * Gets the minimum value of this property. This can be defined by the class
   * property that it implements, or overridden by the instance of the property
   * itself.
   *
   * This is only applicable to scalar, vecN and matN properties. It represents
   * the component-wise minimum of all property values with normalization,
   * offset, and scale applied. If a minimum value is not defined or
   * applicable, this returns an empty value.
   *
   * @return The minimum value of the property.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTableProperty")
  static FCesiumMetadataValue
  GetMinimumValue(UPARAM(ref) const FCesiumPropertyTableProperty& Property);

  /**
   * Gets the maximum value of this property. This can be defined by the class
   * property that it implements, or overridden by the instance of the property
   * itself.
   *
   * This is only applicable to scalar, vecN and matN properties. It represents
   * the component-wise maximum of all property values with normalization,
   * offset, and scale applied. If a maximum value is not defined or applicable,
   * this returns an empty value.
   *
   * @return The maximum value of the property.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTableProperty")
  static FCesiumMetadataValue
  GetMaximumValue(UPARAM(ref) const FCesiumPropertyTableProperty& Property);

  /**
   * Gets the "no data" value of this property, as defined by its class
   * property. This value functions a sentinel value, indicating missing data
   * wherever it appears. The value is compared against the property's raw data,
   * without normalization, offset, or scale applied.
   *
   * This is not applicable to boolean properties. If a "no data" value is
   * not defined or applicable, this returns an empty value.
   *
   * @return The "no data" value of the property.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTableProperty")
  static FCesiumMetadataValue
  GetNoDataValue(UPARAM(ref) const FCesiumPropertyTableProperty& Property);

  /**
   * Gets the default value of this property, as defined by its class
   * property. This default value is used use when encountering a "no data"
   * value in the property.
   *
   * If a default value is not defined, this returns an empty value.
   *
   * @return The default value of the property.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTableProperty")
  static FCesiumMetadataValue
  GetDefaultValue(UPARAM(ref) const FCesiumPropertyTableProperty& Property);
};
