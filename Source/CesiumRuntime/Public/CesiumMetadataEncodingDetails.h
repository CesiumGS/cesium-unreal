// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumMetadataValueType.h"
#include <cstdlib>
#include <type_traits>

#include "CesiumMetadataEncodingDetails.generated.h"

/**
 * @brief The component type that a metadata property's values will be encoded
 * as. These correspond to the pixel component types that are supported in
 * Unreal textures.
 */
UENUM()
enum class ECesiumEncodedMetadataComponentType : uint8 { None, Uint8, Float };

/**
 * @brief The type that a metadata property's values will be encoded as.
 */
UENUM()
enum class ECesiumEncodedMetadataType : uint8 {
  None,
  Scalar,
  Vec2,
  Vec3,
  Vec4
};

/**
 * @brief Indicates how a property value from EXT_structural_metadata should be
 * converted to a GPU-accessible type, if possible.
 */
UENUM()
enum class ECesiumEncodedMetadataConversion : uint8 {
  /**
   * Do nothing. This is typically used for property types that are
   * completely unable to be coerced.
   */
  None,
  /**
   * Coerce the components of a property value to the specified component type.
   * If the property contains string values, this attempts to parse numbers from
   * the strings as uint8s.
   */
  Coerce,
  /**
   * Attempt to parse a color from a string property value. This supports
   * the following formats:
   * - rgb(R, G, B), where R, G, and B are values in the range [0, 255]
   * - hexcode colors, e.g. #ff0000
   */
  ParseColorFromString
};

/**
 * Describes how a property from EXT_structural_metadata will be encoded for
 * access in Unreal materials.
 */
USTRUCT()
struct FCesiumMetadataEncodingDetails {
  GENERATED_USTRUCT_BODY()

  FCesiumMetadataEncodingDetails()
      : Type(ECesiumEncodedMetadataType::None),
        ComponentType(ECesiumEncodedMetadataComponentType::None),
        Conversion(ECesiumEncodedMetadataConversion::None) {}

  FCesiumMetadataEncodingDetails(
      ECesiumEncodedMetadataType InType,
      ECesiumEncodedMetadataComponentType InComponentType,
      ECesiumEncodedMetadataConversion InConversion)
      : Type(InType),
        ComponentType(InComponentType),
        Conversion(InConversion) {}

  /**
   * The GPU-compatible type that this property's values will be encoded as.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  ECesiumEncodedMetadataType Type;

  /**
   * The GPU-compatible component type that this property's values will be
   * encoded as. These correspond to the pixel component types that are
   * supported in Unreal textures.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  ECesiumEncodedMetadataComponentType ComponentType;

  /**
   * The method of conversion used for this property. This describes how the
   * values will be converted for access in Unreal materials. Note that not all
   * property types are compatible with the methods of conversion.
   */
  UPROPERTY(EditAnywhere, Category = "Cesium")
  ECesiumEncodedMetadataConversion Conversion;

  inline bool operator==(const FCesiumMetadataEncodingDetails& Info) const {
    return Type == Info.Type && ComponentType == Info.ComponentType &&
           Conversion == Info.Conversion;
  }

  inline bool operator!=(const FCesiumMetadataEncodingDetails& Info) const {
    return Type != Info.Type || ComponentType != Info.ComponentType ||
           Conversion != Info.Conversion;
  }

  bool HasValidType() const {
    return Type != ECesiumEncodedMetadataType::None &&
           ComponentType != ECesiumEncodedMetadataComponentType::None;
  }
};
