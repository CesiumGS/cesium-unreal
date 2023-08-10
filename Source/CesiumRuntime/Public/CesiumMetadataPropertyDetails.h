// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumMetadataValueType.h"
#include "UObject/ObjectMacros.h"

#include "CesiumMetadataPropertyDetails.generated.h"

/**
 * Represents information about a metadata property according to how the
 * property is defined in EXT_structural_metadata.
 */
USTRUCT(BlueprintType)
struct CESIUMRUNTIME_API FCesiumMetadataPropertyDetails {
  GENERATED_USTRUCT_BODY()

  FCesiumMetadataPropertyDetails()
      : Type(ECesiumMetadataType::Invalid),
        ComponentType(ECesiumMetadataComponentType::None),
        bIsArray(false) {}

  FCesiumMetadataPropertyDetails(
      ECesiumMetadataType InType,
      ECesiumMetadataComponentType InComponentType,
      bool IsArray)
      : Type(InType), ComponentType(InComponentType), bIsArray(IsArray) {}

  /**
   * The type of the metadata property.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  ECesiumMetadataType Type;

  /**
   * The component of the metadata property. Only applies when the type is a
   * Scalar, VecN, or MatN type.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      Meta =
          (EditCondition =
               "Type != ECesiumMetadataType::Invalid && Type != ECesiumMetadataType::Boolean && Type != ECesiumMetadataType::Enum && Type != ECesiumMetadataType::String"))
  ECesiumMetadataComponentType ComponentType;

  /**
   * Whether or not this represents an array containing elements of the
   * specified types.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  bool bIsArray;

  /**
   * The size of the arrays in the metadata property. If the property contains
   * arrays of varying length, this will be zero even though bIsArray will be
   * true> If this property does not contain arrays, this is set to zero.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      Meta = (EditCondition = "bIsArray"))
  int64 ArraySize = 0;

  /**
   * Whether or not the values in this property are normalized. Only applicable
   * to scalar, vecN, and matN types with integer components.
   *
   * For unsigned integer component types, values are normalized between
   * [0.0, 1.0]. For signed integer component types, values are normalized
   * between [-1.0, 1.0]
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium",
      Meta =
          (EditCondition =
               "(Type != ECesiumMetadataType::Invalid && Type != ECesiumMetadataType::Boolean && Type != ECesiumMetadataType::Enum && Type != ECesiumMetadataType::String) && (ComponentType != ECesiumMetadataComponentType::None && ComponentType != ECesiumMetadataComponentType::Float32 && ComponentType != ECesiumMetadataComponentType::Float64)"))
  bool bIsNormalized;

  // TODO: scale and offset, no data + default value

  inline bool
  operator==(const FCesiumMetadataPropertyDetails& ValueType) const {
    return Type == ValueType.Type && ComponentType == ValueType.ComponentType &&
           bIsArray == ValueType.bIsArray;
  }

  inline bool
  operator!=(const FCesiumMetadataPropertyDetails& ValueType) const {
    return Type != ValueType.Type || ComponentType != ValueType.ComponentType ||
           bIsArray != ValueType.bIsArray;
  }

  /**
   * Returns the internal types as a FCesiumMetadataValueType.
   */
  FCesiumMetadataValueType GetValueType() const {
    return FCesiumMetadataValueType(Type, ComponentType, bIsArray);
  }

  /**
   * Sets the internal types to the values supplied by the input
   * FCesiumMetadataValueType.
   */
  void SetValueType(FCesiumMetadataValueType ValueType) {
    Type = ValueType.Type;
    ComponentType = ValueType.ComponentType;
    bIsArray = ValueType.bIsArray;
  }
};
