// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumMetadataEnum.h"
#include "CesiumMetadataValue.h"
#include "CesiumMetadataValueType.h"
#include "CesiumPropertyArray.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/ObjectMacros.h"

#include <CesiumGltf/PropertyAttributePropertyView.h>
#include <CesiumGltf/PropertyTypeTraits.h>
#include <any>
#include <span>

#include "CesiumPropertyAttributeProperty.generated.h"

/**
 * @brief Reports the status of a FCesiumPropertyAttributeProperty. If the
 * property attribute property cannot be accessed, this briefly indicates why.
 */
UENUM(BlueprintType)
enum class ECesiumPropertyAttributePropertyStatus : uint8 {
  /* The property attribute property is valid. */
  Valid = 0,
  /* The property attribute property is empty but has a specified default value.
   */
  EmptyPropertyWithDefault,
  /* The property attribute property does not exist in the glTF, or the property
     definition itself contains errors. */
  ErrorInvalidProperty,
  /* The data associated with the property attribute property is malformed and
     cannot be retrieved. */
  ErrorInvalidPropertyData
};

/**
 * A Blueprint-accessible wrapper for a glTF property attribute property in
 * EXT_structural_metadata. Provides per-vertex access to metadata encoded in a
 * glTF primitive's vertices.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumPropertyAttributeProperty {
  GENERATED_USTRUCT_BODY()

public:
  /**
   * Construct an invalid property with an unknown type.
   */
  FCesiumPropertyAttributeProperty()
      : _status(ECesiumPropertyAttributePropertyStatus::ErrorInvalidProperty),
        _property(),
        _valueType(),
        _normalized(false) {}

  /**
   * Construct a wrapper for the property attribute property view.
   *
   * @param Property The PropertyAttributePropertyView to be stored in this
   * struct.
   */
  template <typename T, bool Normalized>
  FCesiumPropertyAttributeProperty(
      const CesiumGltf::PropertyAttributePropertyView<T, Normalized>& Property)
      : FCesiumPropertyAttributeProperty(
            Property,
            TSharedPtr<FCesiumMetadataEnum>(nullptr)) {}

  /**
   * Construct a wrapper for the property attribute property view.
   *
   * @param Property The PropertyAttributePropertyView to be stored in this
   * struct.
   * @param EnumDefinition The enum definition to use, if any.
   */
  template <typename T, bool Normalized>
  FCesiumPropertyAttributeProperty(
      const CesiumGltf::PropertyAttributePropertyView<T, Normalized>& Property,
      const TSharedPtr<FCesiumMetadataEnum>& EnumDefinition)
      : _status(ECesiumPropertyAttributePropertyStatus::ErrorInvalidProperty),
        _property(Property),
        _valueType(),
        _normalized(Normalized),
        _pEnumDefinition(EnumDefinition) {
    switch (Property.status()) {
    case CesiumGltf::PropertyAttributePropertyViewStatus::Valid:
      _status = ECesiumPropertyAttributePropertyStatus::Valid;
      break;
    case CesiumGltf::PropertyAttributePropertyViewStatus::
        EmptyPropertyWithDefault:
      _status =
          ECesiumPropertyAttributePropertyStatus::EmptyPropertyWithDefault;
      break;
    case CesiumGltf::PropertyAttributePropertyViewStatus::
        ErrorInvalidPropertyAttribute:
    case CesiumGltf::PropertyAttributePropertyViewStatus::
        ErrorNonexistentProperty:
    case CesiumGltf::PropertyAttributePropertyViewStatus::ErrorTypeMismatch:
    case CesiumGltf::PropertyAttributePropertyViewStatus::
        ErrorComponentTypeMismatch:
    case CesiumGltf::PropertyAttributePropertyViewStatus::
        ErrorArrayTypeMismatch:
    case CesiumGltf::PropertyAttributePropertyViewStatus::
        ErrorInvalidNormalization:
    case CesiumGltf::PropertyAttributePropertyViewStatus::
        ErrorNormalizationMismatch:
    case CesiumGltf::PropertyAttributePropertyViewStatus::ErrorInvalidOffset:
    case CesiumGltf::PropertyAttributePropertyViewStatus::ErrorInvalidScale:
    case CesiumGltf::PropertyAttributePropertyViewStatus::ErrorInvalidMax:
    case CesiumGltf::PropertyAttributePropertyViewStatus::ErrorInvalidMin:
    case CesiumGltf::PropertyAttributePropertyViewStatus::
        ErrorInvalidNoDataValue:
    case CesiumGltf::PropertyAttributePropertyViewStatus::
        ErrorInvalidDefaultValue:
      // The status was already set in the initializer list.
      return;
    default:
      _status =
          ECesiumPropertyAttributePropertyStatus::ErrorInvalidPropertyData;
      return;
    }

    _valueType = TypeToMetadataValueType<T>(EnumDefinition);
    _normalized = Normalized;
  }

  /**
   * @brief Gets the stride of the underlying accessor.
   */
  int64 getAccessorStride() const;

  /**
   * @brief Gets a pointer to the first byte of the underlying accessor's data.
   */
  const std::byte* getAccessorData() const;

private:
  ECesiumPropertyAttributePropertyStatus _status;

  std::any _property;

  FCesiumMetadataValueType _valueType;
  bool _normalized;
  TSharedPtr<FCesiumMetadataEnum> _pEnumDefinition;

  friend class UCesiumPropertyAttributePropertyBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumPropertyAttributePropertyBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Gets the status of the property attribute property. If this property
   * attribute property is invalid in any way, this will briefly indicate why.
   *
   * @param Property The property attribute property.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyAttributeProperty")
  static ECesiumPropertyAttributePropertyStatus
  GetPropertyAttributePropertyStatus(
      UPARAM(ref) const FCesiumPropertyAttributeProperty& Property);

  /**
   * Gets the best-fitting type for the property that is accessible from
   * Blueprints. For the most precise representation of the values possible in
   * Blueprints, you should retrieve it using this type.
   *
   * @param Property The property attribute property.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyAttributeProperty")
  static ECesiumMetadataBlueprintType
  GetBlueprintType(UPARAM(ref)
                       const FCesiumPropertyAttributeProperty& Property);

  /**
   * Gets the type of the metadata value as defined in the
   * EXT_structural_metadata extension. Many of these types are not accessible
   * from Blueprints, but can be converted to a Blueprint-accessible type.
   *
   * @param Property The property attribute property.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyAttributeProperty")
  static FCesiumMetadataValueType
  GetValueType(UPARAM(ref) const FCesiumPropertyAttributeProperty& Property);

  /**
   * Gets the number of values in the property.
   *
   * @param Property The property attribute property.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyAttributeProperty")
  static int64
  GetPropertySize(UPARAM(ref) const FCesiumPropertyAttributeProperty& Property);

  /**
   * Attempts to retrieve the value at the given index as an unsigned 8-bit
   * integer.
   *
   * For numeric properties, the raw value will be transformed by the property's
   * normalization, scale, and offset before it is further converted. If the raw
   * value is equal to the property's "no data" value, then the property's
   * default value will be converted if possible. If the property-defined
   * default value cannot be converted, or does not exist, then the user-defined
   * default value is returned.
   *
   * If the value is an integer between 0 and 255, it is returned as-is.
   * Otherwise, if the value is a floating-point number in the aforementioned
   * range, it is truncated (rounded toward zero) and returned.
   *
   * In all other cases, the user-defined default value is returned. If the
   * index is out-of-range, or if the property attribute property is somehow
   * invalid, the user-defined default value is returned.
   *
   * @param Property The property attribute property.
   * @param Index The index.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as a Byte.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyAttributeProperty")
  static uint8 GetByte(
      UPARAM(ref) const FCesiumPropertyAttributeProperty& Property,
      int64 Index,
      uint8 DefaultValue = 0);

  /**
   * Attempts to retrieve the value for the given index as a signed 32-bit
   * integer.
   *
   * For numeric properties, the raw value will be transformed by the property's
   * normalization, scale, and offset before it is further converted. If the raw
   * value is equal to the property's "no data" value, then the property's
   * default value will be converted if possible. If the property-defined
   * default value cannot be converted, or does not exist, then the user-defined
   * default value is returned.
   *
   * If the value is an integer between -2,147,483,648 and 2,147,483,647, it
   * is returned as-is. Otherwise, if the value is a floating-point number in
   * the aforementioned range, it is truncated (rounded toward zero) and
   * returned.
   *
   * In all other cases, the user-defined default value is returned. If the
   * index is out-of-range, or if the property attribute property is
   * somehow invalid, the user-defined default value is returned.
   *
   * @param Property The property attribute property.
   * @param Index The index.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as an Integer.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyAttributeProperty")
  static int32 GetInteger(
      UPARAM(ref) const FCesiumPropertyAttributeProperty& Property,
      int64 Index,
      int32 DefaultValue = 0);

  /**
   * Attempts to retrieve the value for the given index as a signed 64-bit
   * integer.
   *
   * Although property attribute properties do not directly support
   * 64-bit integers, this can be used to losslessly retrieve values from
   * unsigned 32-bit integer properties.
   *
   * For numeric properties, the raw value will be transformed by the property's
   * normalization, scale, and offset before it is further converted. If the raw
   * value is equal to the property's "no data" value, then the property's
   * default value will be converted if possible. If the property-defined
   * default value cannot be converted, or does not exist, then the user-defined
   * default value is returned.
   *
   * If the value is an integer and between -2^63 and (2^63 - 1), it is
   * returned as-is. Otherwise, if the value is a floating-point number in the
   * aforementioned range, it is truncated (rounded toward zero) and returned.
   *
   * In all other cases, the user-defined default value is returned. If the
   * index ID is out-of-range, or if the property table property is somehow
   * invalid, the user-defined default value is returned.
   *
   * @param Property The property table property.
   * @param Index The index.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as an Integer64.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyAttributeProperty")
  static int64 GetInteger64(
      UPARAM(ref) const FCesiumPropertyAttributeProperty& Property,
      int64 Index,
      int64 DefaultValue = 0);

  /**
   * Attempts to retrieve the value for the given index as a single-precision
   * floating-point number.
   *
   * For numeric properties, the raw value will be transformed by the property's
   * normalization, scale, and offset before it is further converted. If the raw
   * value is equal to the property's "no data" value, then the property's
   * default value will be converted if possible. If the property-defined
   * default value cannot be converted, or does not exist, then the user-defined
   * default value is returned.
   *
   * If the value is already a single-precision floating-point number, it is
   * returned as-is. Otherwise, if the value is a scalar of any other type
   * within the range of values that a single-precision float can represent, it
   * is converted to its closest representation as a single-precision float and
   * returned.
   *
   * In all other cases, the user-defined default value is returned. If the
   * index is out-of-range, or if the property attribute property is
   * somehow invalid, the user-defined default value is returned.
   *
   * @param Property The property attribute property.
   * @param Index The index.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as a Float.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyAttributeProperty")
  static float GetFloat(
      UPARAM(ref) const FCesiumPropertyAttributeProperty& Property,
      int64 Index,
      float DefaultValue = 0.0f);

  /**
   * Attempts to retrieve the value for the given index as a double-precision
   * floating-point number.
   *
   * For numeric properties, the raw value will be transformed by the property's
   * normalization, scale, and offset before it is further converted. If the raw
   * value is equal to the property's "no data" value, then the property's
   * default value will be converted if possible. If the property-defined
   * default value cannot be converted, or does not exist, then the user-defined
   * default value is returned.
   *
   * If the value is a single-precision floating-point number, it is returned
   * as-is. Otherwise, if the value is an integer, it is converted to the
   * closest representation as a double-precision floating-point number.
   *
   * In all other cases, the user-defined default value is returned. If the
   * index is out-of-range, or if the property attribute property is
   * somehow invalid, the user-defined default value is returned.
   *
   * @param Property The property attribute property.
   * @param Index The index.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as a Float64.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyAttributeProperty")
  static double GetFloat64(
      UPARAM(ref) const FCesiumPropertyAttributeProperty& Property,
      int64 Index,
      double DefaultValue = 0.0);

  /**
   * Attempts to retrieve the value for the given index as a FIntPoint.
   *
   * For numeric properties, the raw value will be transformed by the property's
   * normalization, scale, and offset before it is further converted. If the raw
   * value is equal to the property's "no data" value, then the property's
   * default value will be converted if possible. If the property-defined
   * default value cannot be converted, or does not exist, then the user-defined
   * default value is returned.
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
   * In all other cases, the user-defined default value is returned. In all
   * vector cases, if any of the relevant components cannot be represented as a
   * 32-bit signed, the default value is returned.
   *
   * If the index is out-of-range, or if the property attribute property is
   * somehow invalid, the user-defined default value is returned.
   *
   * @param Property The property attribute property.
   * @param Index The index.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as a FIntPoint.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyAttributeProperty")
  static FIntPoint GetIntPoint(
      UPARAM(ref) const FCesiumPropertyAttributeProperty& Property,
      int64 Index,
      const FIntPoint& DefaultValue);

  /**
   * Attempts to retrieve the value for the given index as a FVector2D.
   *
   * For numeric properties, the raw value will be transformed by the property's
   * normalization, scale, and offset before it is further converted. If the raw
   * value is equal to the property's "no data" value, then the property's
   * default value will be converted if possible. If the property-defined
   * default value cannot be converted, or does not exist, then the user-defined
   * default value is returned.
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
   * In all other cases, the user-defined default value is returned. If the
   * index is out-of-range, or if the property attribute property is
   * somehow invalid, the user-defined default value is returned.
   *
   * @param Property The property attribute property.
   * @param Index The index.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as a FVector2D.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyAttributeProperty")
  static FVector2D GetVector2D(
      UPARAM(ref) const FCesiumPropertyAttributeProperty& Property,
      int64 Index,
      const FVector2D& DefaultValue);

  /**
   * Attempts to retrieve the value for the given index as a FIntVector.
   *
   * For numeric properties, the raw value will be transformed by the property's
   * normalization, scale, and offset before it is further converted. If the raw
   * value is equal to the property's "no data" value, then the property's
   * default value will be converted if possible. If the property-defined
   * default value cannot be converted, or does not exist, then the user-defined
   * default value is returned.
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
   * In all other cases, the user-defined default value is returned. In all
   * vector cases, if any of the relevant components cannot be represented as a
   * 32-bit signed integer, the default value is returned.
   *
   * If the index is out-of-range, or if the property attribute property is
   * somehow invalid, the user-defined default value is returned.
   *
   * @param Property The property attribute property.
   * @param Index The index.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as a FIntVector.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyAttributeProperty")
  static FIntVector GetIntVector(
      UPARAM(ref) const FCesiumPropertyAttributeProperty& Property,
      int64 Index,
      const FIntVector& DefaultValue);

  /**
   * Attempts to retrieve the value for the given index as a FVector3f.
   *
   * For numeric properties, the raw value transformed by the property's
   * normalization, scale, and offset before it is further converted. If the raw
   * value is equal to the property's "no data" value, then the property's
   * default value will be converted if possible. If the property-defined
   * default value cannot be converted, or does not exist, then the user-defined
   * default value is returned.
   *
   * Property values are converted as follows:
   *
   * - If the value is a 3-dimensional vector, its components will be converted
   * to the closest represenattribute single-precision floats, if possible.
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
   * In all other cases, the user-defined default value is returned. In all
   * vector cases, if any of the relevant components cannot be represented as a
   * single-precision float, the user-defined default value is returned.
   *
   * If the index is out-of-range, or if the property attribute property is
   * somehow invalid, the user-defined default value is returned.
   *
   * @param Property The property attribute property.
   * @param Index The index.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as a FVector3f.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyAttributeProperty")
  static FVector3f GetVector3f(
      UPARAM(ref) const FCesiumPropertyAttributeProperty& Property,
      int64 Index,
      const FVector3f& DefaultValue);

  /**
   * Attempts to retrieve the value for the given index as a FVector.
   *
   * For numeric properties, the raw value will be transformed by the property's
   * normalization, scale, and offset before it is further converted. If the raw
   * value is equal to the property's "no data" value, then the property's
   * default value will be converted if possible. If the property-defined
   * default value cannot be converted, or does not exist, then the user-defined
   * default value is returned.
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
   * In all other cases, the user-defined default value is returned. If the
   * index is out-of-range, or if the property attribute property is
   * somehow invalid, the user-defined default value is returned.
   *
   * @param Property The property attribute property.
   * @param Index The index.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as a FVector.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyAttributeProperty")
  static FVector GetVector(
      UPARAM(ref) const FCesiumPropertyAttributeProperty& Property,
      int64 Index,
      const FVector& DefaultValue);

  /**
   * Attempts to retrieve the value for the given index as a FVector4.
   *
   * For numeric properties, the raw value will be transformed by the property's
   * normalization, scale, and offset before it is further converted. If the raw
   * value is equal to the property's "no data" value, then the property's
   * default value will be converted if possible. If the property-defined
   * default value cannot be converted, or does not exist, then the user-defined
   * default value is returned.
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
   * In all other cases, the user-defined default value is returned. If the
   * index is out-of-range, or if the property attribute property is
   * somehow invalid, the user-defined default value is returned.
   *
   * @param Property The property attribute property.
   * @param Index The index.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as a FVector4.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyAttributeProperty")
  static FVector4 GetVector4(
      UPARAM(ref) const FCesiumPropertyAttributeProperty& Property,
      int64 Index,
      const FVector4& DefaultValue);

  /**
   * Attempts to retrieve the value for the given index as a FMatrix.
   *
   * For numeric properties, the raw value will be transformed by the property's
   * normalization, scale, and offset before it is further converted. If the raw
   * value is equal to the property's "no data" value, then the property's
   * default value will be converted if possible. If the property-defined
   * default value cannot be converted, or does not exist, then the user-defined
   * default value is returned.
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
   * In all other cases, the user-defined default value is returned. If the
   * index is out-of-range, or if the property attribute property is
   * somehow invalid, the user-defined default value is returned.
   *
   * @param Property The property attribute property.
   * @param Index The index.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as a FMatrix.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyAttributeProperty")
  static FMatrix GetMatrix(
      UPARAM(ref) const FCesiumPropertyAttributeProperty& Property,
      int64 Index,
      const FMatrix& DefaultValue);

  /**
   * Retrieves the value of the property for the given index. This allows the
   * value to be acted on more generically; its true value can be retrieved
   * later as a specific Blueprints type.
   *
   * For numeric properties, the raw value will be transformed by the property's
   * normalization, scale, and offset before it is returned. If the raw value is
   * equal to the property's "no data" value, an empty value will be returned.
   * However, if the property itself specifies a default value, then the
   * property-defined default value will be returned.
   *
   * @param Property The property attribute property.
   * @param Index The index.
   * @return The property value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyAttributeProperty")
  static FCesiumMetadataValue GetValue(
      UPARAM(ref) const FCesiumPropertyAttributeProperty& Property,
      int64 Index);

  /**
   * Retrieves the raw value of the property for the given index. This is the
   * value of the property without normalization, offset, or scale applied.
   *
   * If this property specifies a "no data" value, and the raw value is equal to
   * this "no data" value, the value is returned as-is.
   *
   * If this property is an empty property with a specified default value, it
   * will not have any raw data to retrieve. The returned value will be empty.

   * @param Property The property attribute property.
   * @param Index The index.
   * @return The raw property value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyAttributeProperty")
  static FCesiumMetadataValue GetRawValue(
      UPARAM(ref) const FCesiumPropertyAttributeProperty& Property,
      int64 Index);

  /**
   * Whether this property is normalized. Only applicable when this property has
   * an integer component type.
   *
   * @param Property The property attribute property.
   * @return Whether this property is normalized.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyAttributeProperty")
  static bool
  IsNormalized(UPARAM(ref) const FCesiumPropertyAttributeProperty& Property);

  /**
   * Gets the offset of this property. This can be defined by the class property
   * that it implements, or overridden by the instance of the property itself.
   *
   * This is only applicable to properties with floating-point or normalized
   * integer component types. If an offset is not defined or applicable, this
   * returns an empty value.
   *
   * @param Property The property attribute property.
   * @return The offset of the property.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyAttributeProperty")
  static FCesiumMetadataValue
  GetOffset(UPARAM(ref) const FCesiumPropertyAttributeProperty& Property);

  /**
   * Gets the scale of this property. This can be defined by the class property
   * that it implements, or overridden by the instance of the property itself.
   *
   * This is only applicable to properties with floating-point or normalized
   * integer component types. If a scale is not defined or applicable, this
   * returns an empty value.
   *
   * @param Property The property attribute property.
   * @return The scale of the property.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyAttributeProperty")
  static FCesiumMetadataValue
  GetScale(UPARAM(ref) const FCesiumPropertyAttributeProperty& Property);

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
   * @param Property The property attribute property.
   * @return The minimum value of the property.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyAttributeProperty")
  static FCesiumMetadataValue
  GetMinimumValue(UPARAM(ref) const FCesiumPropertyAttributeProperty& Property);

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
   * @param Property The property attribute property.
   * @return The maximum value of the property.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyAttributeProperty")
  static FCesiumMetadataValue
  GetMaximumValue(UPARAM(ref) const FCesiumPropertyAttributeProperty& Property);

  /**
   * Gets the "no data" value of this property, as defined by its class
   * property. This value functions a sentinel value, indicating missing data
   * wherever it appears. The value is compared against the property's raw data,
   * without normalization, offset, or scale applied.
   *
   * If a "no data" value is not defined or applicable, this returns an empty
   * value.
   *
   * @param Property The property attribute property.
   * @return The "no data" value of the property.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyAttributeProperty")
  static FCesiumMetadataValue
  GetNoDataValue(UPARAM(ref) const FCesiumPropertyAttributeProperty& Property);

  /**
   * Gets the default value of this property, as defined by its class
   * property. This default value is used use when encountering a "no data"
   * value in the property.
   *
   * If a default value is not defined, this returns an empty value.
   *
   * @param Property The property attribute property.
   * @return The default value of the property.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyAttributeProperty")
  static FCesiumMetadataValue
  GetDefaultValue(UPARAM(ref) const FCesiumPropertyAttributeProperty& Property);
};
