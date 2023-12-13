// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/PropertyTexturePropertyView.h"
#include "CesiumMetadataValue.h"
#include "GenericPlatform/GenericPlatform.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include <any>
#include "CesiumPropertyTextureProperty.generated.h"

/**
 * @brief Reports the status of a FCesiumPropertyTextureProperty. If the
 * property texture property cannot be accessed, this briefly indicates why.
 */
UENUM(BlueprintType)
enum class ECesiumPropertyTexturePropertyStatus : uint8 {
  /* The property texture property is valid. */
  Valid = 0,
  /* The property texture property is empty but has a specified default value.
   */
  EmptyPropertyWithDefault,
  /* The property texture property does not exist in the glTF, or the property
     definition itself contains errors. */
  ErrorInvalidProperty,
  /* The data associated with the property texture property is malformed and
     cannot be retrieved. */
  ErrorInvalidPropertyData,
  /* The type of this property texture property is not supported. */
  ErrorUnsupportedProperty
};

/**
 * @brief A blueprint-accessible wrapper for a property texture property from a
 * glTF. Provides per-pixel access to metadata encoded in a property texture.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumPropertyTextureProperty {
  GENERATED_USTRUCT_BODY()

public:
  FCesiumPropertyTextureProperty()
      : _status(ECesiumPropertyTexturePropertyStatus::ErrorInvalidProperty),
        _property(),
        _valueType(),
        _normalized(false) {}

  template <typename T, bool Normalized>
  FCesiumPropertyTextureProperty(
      const CesiumGltf::PropertyTexturePropertyView<T, Normalized>& Property)
      : _status(ECesiumPropertyTexturePropertyStatus::ErrorInvalidProperty),
        _property(Property),
        _valueType(),
        _normalized(Normalized) {
    switch (Property.status()) {
    case CesiumGltf::PropertyTexturePropertyViewStatus::Valid:
      _status = ECesiumPropertyTexturePropertyStatus::Valid;
      break;
    case CesiumGltf::PropertyTexturePropertyViewStatus::
        EmptyPropertyWithDefault:
      _status = ECesiumPropertyTexturePropertyStatus::EmptyPropertyWithDefault;
      break;
    case CesiumGltf::PropertyTexturePropertyViewStatus::
        ErrorUnsupportedProperty:
      _status = ECesiumPropertyTexturePropertyStatus::ErrorUnsupportedProperty;
      return;
    case CesiumGltf::PropertyTexturePropertyViewStatus::
        ErrorInvalidPropertyTexture:
    case CesiumGltf::PropertyTexturePropertyViewStatus::
        ErrorNonexistentProperty:
    case CesiumGltf::PropertyTexturePropertyViewStatus::ErrorTypeMismatch:
    case CesiumGltf::PropertyTexturePropertyViewStatus::
        ErrorComponentTypeMismatch:
    case CesiumGltf::PropertyTexturePropertyViewStatus::ErrorArrayTypeMismatch:
    case CesiumGltf::PropertyTexturePropertyViewStatus::
        ErrorInvalidNormalization:
    case CesiumGltf::PropertyTexturePropertyViewStatus::
        ErrorNormalizationMismatch:
    case CesiumGltf::PropertyTexturePropertyViewStatus::ErrorInvalidOffset:
    case CesiumGltf::PropertyTexturePropertyViewStatus::ErrorInvalidScale:
    case CesiumGltf::PropertyTexturePropertyViewStatus::ErrorInvalidMax:
    case CesiumGltf::PropertyTexturePropertyViewStatus::ErrorInvalidMin:
    case CesiumGltf::PropertyTexturePropertyViewStatus::ErrorInvalidNoDataValue:
    case CesiumGltf::PropertyTexturePropertyViewStatus::
        ErrorInvalidDefaultValue:
      // The status was already set in the initializer list.
      return;
    default:
      _status = ECesiumPropertyTexturePropertyStatus::ErrorInvalidPropertyData;
      return;
    }

    _valueType = TypeToMetadataValueType<T>();
    _normalized = Normalized;
  }

  const int64 getTexCoordSetIndex() const;
  const CesiumGltf::Sampler* getSampler() const;
  const CesiumGltf::ImageCesium* getImage() const;

private:
  ECesiumPropertyTexturePropertyStatus _status;

  std::any _property;

  FCesiumMetadataValueType _valueType;
  bool _normalized;

  friend class UCesiumPropertyTexturePropertyBlueprintLibrary;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumPropertyTexturePropertyBlueprintLibrary
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Gets the status of the property texture property. If this property texture
   * property is invalid in any way, this will briefly indicate why.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTextureProperty")
  static ECesiumPropertyTexturePropertyStatus GetPropertyTexturePropertyStatus(
      UPARAM(ref) const FCesiumPropertyTextureProperty& Property);

  /**
   * Gets the best-fitting type for the property that is accessible from
   * Blueprints. For the most precise representation of the values possible in
   * Blueprints, you should retrieve it using this type.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTextureProperty")
  static ECesiumMetadataBlueprintType
  GetBlueprintType(UPARAM(ref) const FCesiumPropertyTextureProperty& Property);

  /**
   * Gets the best-fitting Blueprints type for the elements in this property's
   * array values. If the given property does not contain array values, this
   * returns None.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTextureProperty")
  static ECesiumMetadataBlueprintType GetArrayElementBlueprintType(
      UPARAM(ref) const FCesiumPropertyTextureProperty& Property);

  /**
   * Gets the type of the metadata value as defined in the
   * EXT_structural_metadata extension. Many of these types are not accessible
   * from Blueprints, but can be converted to a Blueprint-accessible type.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTextureProperty")
  static FCesiumMetadataValueType
  GetValueType(UPARAM(ref) const FCesiumPropertyTextureProperty& Property);

  /**
   * Gets the number of elements in an array of this property. Only
   * applicable when the property is a fixed-length array type.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTextureProperty")
  static int64 GetArraySize(UPARAM(ref)
                                const FCesiumPropertyTextureProperty& Property);

  /**
   * Gets the glTF texture coordinate set index used by the property texture
   * property. This is the index N corresponding to the "TEXCOORD_N" attribute
   * on the glTF primitive that samples this texture.
   *
   * If the property texture property is invalid, this returns -1.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTextureProperty")
  static int64 GetGltfTextureCoordinateSetIndex(
      UPARAM(ref) const FCesiumPropertyTextureProperty& Property);

  /**
   * Gets the UV channel containing the texture coordinate set that is used by
   * the property texture property on the given component. This refers to the UV
   * channel it uses on the primitive's static mesh, which is not necessarily
   * equal to value of GetGltfTextureCoordinateSetIndex.
   *
   * This function may be used with FindCollisionUV to get the feature ID from a
   * line trace hit. However, in order for this function to work, the feature ID
   * texture should be listed under the CesiumFeaturesMetadataComponent of the
   * owner Cesium3DTileset. Otherwise, its texture coordinate set may not be
   * included in the Unreal mesh data. To avoid using
   * CesiumFeaturesMetadataComponent, use GetFeatureIDFromHit instead.
   *
   * This returns -1 if the property texture property is invalid, or if the
   * specified texture coordinate set is not present in the component's mesh
   * data.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTextureProperty")
  static int64 GetUnrealUVChannel(
      const UPrimitiveComponent* Component,
      UPARAM(ref) const FCesiumPropertyTextureProperty& Property);

  /**
   * @brief Get the channels array of this property. This contains the indices
   * of the meaningful texel channels that will be used when sampling the
   * property texture.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTextureProperty")
  static TArray<int64>
  GetChannels(UPARAM(ref) const FCesiumPropertyTextureProperty& Property);

  /**
   * Attempts to retrieve the value at the given texture coordinates as an
   * unsigned 8-bit integer.
   *
   * For numeric properties, the raw value for the given coordinates will be
   * transformed by the property's normalization, scale, and offset before it is
   * further converted. If the raw value is equal to the property's "no data"
   * value, then the property's default value will be converted if possible. If
   * the property-defined default value cannot be converted, or does not exist,
   * then the user-defined default value is returned.
   *
   * Property values are converted as follows:
   *
   * - If the value is an integer between 0 and 255, it is returned as-is.
   * - If the value is a floating-point number in the aforementioned range, it
   * is truncated (rounded toward zero) and returned.
   *
   * In all other cases, the user-defined default value is returned. If the
   * property texture property is somehow invalid, the user-defined default
   * value is returned.
   *
   * @param UV The texture coordinates.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as a Byte.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTextureProperty")
  static uint8 GetByte(
      UPARAM(ref) const FCesiumPropertyTextureProperty& Property,
      const FVector2D& UV,
      uint8 DefaultValue = 0);

  /**
   * Attempts to retrieve the value at the given texture coordinates as a signed
   * 32-bit integer.
   *
   * For numeric properties, the raw value for the given coordinates will be
   * transformed by the property's normalization, scale, and offset before it is
   * further converted. If the raw value is equal to the property's "no data"
   * value, then the property's default value will be converted if
   * possible. If the property-defined default value cannot be converted, or
   * does not exist, then the user-defined default value is returned.
   *
   * Property values are converted as follows:
   *
   * - If the value is an integer between -2,147,483,648 and 2,147,483,647, it
   * is returned as-is.
   * - If the value is a floating-point number in the aforementioned range, it
   * is truncated (rounded toward zero) and returned.
   *
   * In all other cases, the user-defined default value is returned. If the
   * property texture property is somehow invalid, the user-defined default
   * value is returned.
   *
   * @param UV The texture coordinates.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as an Integer.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTextureProperty")
  static int32 GetInteger(
      UPARAM(ref) const FCesiumPropertyTextureProperty& Property,
      const FVector2D& UV,
      int32 DefaultValue = 0);

  /**
   * Attempts to retrieve the value at the given texture coordinates as a
   * single-precision floating-point number.
   *
   * For numeric properties, the raw value for the given coordinates will be
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
   * In all other cases, the user-defined default value is returned. If the
   * property texture property is somehow invalid, the user-defined default
   * value is returned.
   *
   * @param UV The texture coordinates.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as a Float.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTextureProperty")
  static float GetFloat(
      UPARAM(ref) const FCesiumPropertyTextureProperty& Property,
      const FVector2D& UV,
      float DefaultValue = 0.0f);

  /**
   * Attempts to retrieve the value at the given texture coordinates as a
   * double-precision floating-point number.
   *
   * For numeric properties, the raw value for the given coordinates will be
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
   * In all other cases, the user-defined default value is returned. If the
   * property texture property is somehow invalid, the user-defined default
   * value is returned.
   *
   * @param UV The texture coordinates.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as a Float.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTextureProperty")
  static double GetFloat64(
      UPARAM(ref) const FCesiumPropertyTextureProperty& Property,
      const FVector2D& UV,
      double DefaultValue = 0.0);

  /**
   * Attempts to retrieve the value at the given texture coordinates as a
   * FIntPoint.
   *
   * For numeric properties, the raw value for the given coordinates will be
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
   * In all other cases, the user-defined default value is returned. In all
   * vector cases, if any of the relevant components cannot be represented as a
   * 32-bit signed, the default value is returned.
   *
   * If the property texture property is somehow invalid, the user-defined
   * default value is returned.
   *
   * @param UV The texture coordinates.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as a FIntPoint.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTextureProperty")
  static FIntPoint GetIntPoint(
      UPARAM(ref) const FCesiumPropertyTextureProperty& Property,
      const FVector2D& UV,

      const FIntPoint& DefaultValue);

  /**
   * Attempts to retrieve the value at the given texture coordinates as a
   * FVector2D.
   *
   * For numeric properties, the raw value for the given coordinates will be
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
   * - If the value is a scalar that can be converted to a 32-bit signed
   * integer, the resulting FVector2D will have this value in both of its
   * components.
   *
   * In all other cases, the user-defined default value is returned. If the
   * property texture property is somehow invalid, the user-defined default
   * value is returned.
   *
   * @param UV The texture coordinates.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as a FVector2D.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTextureProperty")
  static FVector2D GetVector2D(
      UPARAM(ref) const FCesiumPropertyTextureProperty& Property,
      const FVector2D& UV,

      const FVector2D& DefaultValue);

  /**
   * Attempts to retrieve the value at the given texture coordinates as a
   * FIntVector.
   *
   * For numeric properties, the raw value for the given coordinates will be
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
   * In all other cases, the user-defined default value is returned. In all
   * vector cases, if any of the relevant components cannot be represented as a
   * 32-bit signed integer, the default value is returned.
   *
   * If the property texture property is somehow invalid, the user-defined
   * default value is returned.
   *
   * @param UV The texture coordinates.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as a FIntVector.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTextureProperty")
  static FIntVector GetIntVector(
      UPARAM(ref) const FCesiumPropertyTextureProperty& Property,
      const FVector2D& UV,

      const FIntVector& DefaultValue);

  /**
   * Attempts to retrieve the value at the given texture coordinates as a
   * FVector.
   *
   * For numeric properties, the raw value for the given coordinates will be
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
   * In all other cases, the user-defined default value is returned. In all
   * vector cases, if any of the relevant components cannot be represented as a
   * single-precision float, the default value is returned.
   *
   * If the property texture property is somehow invalid, the user-defined
   * default value is returned.
   *
   * @param UV The texture coordinates.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as a FVector.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTextureProperty")
  static FVector GetVector(
      UPARAM(ref) const FCesiumPropertyTextureProperty& Property,
      const FVector2D& UV,

      const FVector& DefaultValue);

  /**
   * Attempts to retrieve the value at the given texture coordinates as a
   * FVector4.
   *
   * For numeric properties, the raw value for the given coordinates will be
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
   * In all other cases, the user-defined default value is returned. If the
   * property texture property is somehow invalid, the user-defined default
   * value is returned.
   *
   * @param UV The texture coordinates.
   * @param DefaultValue The default value to fall back on.
   * @return The property value as a FVector4.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTextureProperty")
  static FVector4 GetVector4(
      UPARAM(ref) const FCesiumPropertyTextureProperty& Property,
      const FVector2D& UV,

      const FVector4& DefaultValue);

  /**
   * Attempts to retrieve the value for the given texture coordinates as a
   * FCesiumPropertyArray. If the property is not an array type, this returns an
   * empty array.
   *
   * For numeric array properties, the raw array value for a given coordinates
   * will be transformed by the property's normalization, scale, and offset
   * before it is further converted. If the raw value is equal to the property's
   * "no data" value, then the property's default value will be converted if
   * possible. If the property-defined default value cannot be converted, or
   * does not exist, then the user-defined default value is returned.
   *
   * @param UV The texture coordinates.
   * @return The property value as a FCesiumPropertyArray.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTextureProperty")
  static FCesiumPropertyArray GetArray(
      UPARAM(ref) const FCesiumPropertyTextureProperty& Property,
      const FVector2D& UV);

  /**
   * Retrieves the value of the property for the given texture coordinates. This
   * allows the value to be acted on more generically; its true value can be
   * retrieved later as a specific Blueprints type.
   *
   * For numeric properties, the raw value for a given feature will be
   * transformed by the property's normalization, scale, and offset before it is
   * returned. If the raw value is equal to the property's "no data" value, an
   * empty value will be returned. However, if the property itself specifies a
   * default value, then the property-defined default value will be returned.
   *
   * @param UV The texture coordinates.
   * @return The property value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTextureProperty")
  static FCesiumMetadataValue GetValue(
      UPARAM(ref) const FCesiumPropertyTextureProperty& Property,
      const FVector2D& UV);

  /**
   * Retrieves the raw value of the property for the given feature. This is the
   * value of the property without normalization, offset, or scale applied.
   *
   * If this property specifies a "no data" value, and the raw value is equal to
   * this "no data" value, the value is returned as-is.
   *
   * @param UV The texture coordinates.
   * @return The raw property value.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTextureProperty")
  static FCesiumMetadataValue GetRawValue(
      UPARAM(ref) const FCesiumPropertyTextureProperty& Property,
      const FVector2D& UV);

  /**
   * Whether this property is normalized. Only applicable when this property
   * has an integer component type.
   *
   * @return Whether this property is normalized.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Category = "Cesium|Metadata|PropertyTextureProperty")
  static bool IsNormalized(UPARAM(ref)
                               const FCesiumPropertyTextureProperty& Property);

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
      Category = "Cesium|Metadata|PropertyTextureProperty")
  static FCesiumMetadataValue
  GetOffset(UPARAM(ref) const FCesiumPropertyTextureProperty& Property);

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
      Category = "Cesium|Metadata|PropertyTextureProperty")
  static FCesiumMetadataValue
  GetScale(UPARAM(ref) const FCesiumPropertyTextureProperty& Property);

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
      Category = "Cesium|Metadata|PropertyTextureProperty")
  static FCesiumMetadataValue
  GetMinimumValue(UPARAM(ref) const FCesiumPropertyTextureProperty& Property);

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
      Category = "Cesium|Metadata|PropertyTextureProperty")
  static FCesiumMetadataValue
  GetMaximumValue(UPARAM(ref) const FCesiumPropertyTextureProperty& Property);

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
      Category = "Cesium|Metadata|PropertyTextureProperty")
  static FCesiumMetadataValue
  GetNoDataValue(UPARAM(ref) const FCesiumPropertyTextureProperty& Property);

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
      Category = "Cesium|Metadata|PropertyTextureProperty")
  static FCesiumMetadataValue
  GetDefaultValue(UPARAM(ref) const FCesiumPropertyTextureProperty& Property);

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  /**
   * @brief Get the string representing how the metadata is encoded into a
   * pixel color. This is useful to unpack the correct order of the metadata
   * components from the pixel color.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "Swizzles are no longer hardcoded in Unreal materials. To see what channels the property uses, use GetChannels instead."))
  static FString GetSwizzle(UPARAM(ref)
                                const FCesiumPropertyTextureProperty& Property);

  /**
   * @brief Get the component count of this property. Since the metadata is
   * encoded as pixel color, this is also the number of meaningful channels
   * it will use.
   */
  UFUNCTION(
      BlueprintCallable,
      BlueprintPure,
      Meta =
          (DeprecatedFunction,
           DeprecationMessage =
               "Use GetChannels to get the channels array of a property texture property instead."))
  static int64
  GetComponentCount(UPARAM(ref) const FCesiumPropertyTextureProperty& Property);
  PRAGMA_ENABLE_DEPRECATION_WARNINGS
};
