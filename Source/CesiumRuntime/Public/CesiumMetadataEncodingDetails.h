// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumMetadataValueType.h"
#include <cstdlib>
#include <type_traits>

#include "CesiumMetadataEncodingDetails.generated.h"

struct FCesiumMetadataPropertyDetails;

/**
 * @brief The component type that a metadata property's values will be encoded
 * as. These correspond to the pixel component types that are supported in
 * Unreal textures.
 */
UENUM()
enum class ECesiumEncodedMetadataComponentType : uint8 { None, Uint8, Float };

/**
 * @brief Gets the best-fitting encoded type for the given metadata component
 * type.
 */
ECesiumEncodedMetadataComponentType
CesiumMetadataComponentTypeToEncodingType(ECesiumMetadataComponentType Type);

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
 * @brief Gets the best-fitting encoded type for the given metadata type.
 */
ECesiumEncodedMetadataType
CesiumMetadataTypeToEncodingType(ECesiumMetadataType Type);

/**
 * @brief Gets the number of components associated with the given encoded type.
 * @param type The encoded metadata type.
 */
size_t
CesiumGetEncodedMetadataTypeComponentCount(ECesiumEncodedMetadataType Type);

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
   * - `rgb(R, G, B)`, where R, G, and B are values in the range [0, 255]
   * - hexcode colors, e.g. `#ff0000`
   */
  ParseColorFromString
};

/**
 * Describes how a property from EXT_structural_metadata will be encoded for
 * access in Unreal materials.
 */
USTRUCT()
struct CESIUMRUNTIME_API FCesiumMetadataEncodingDetails {
  GENERATED_USTRUCT_BODY()

  FCesiumMetadataEncodingDetails();

  FCesiumMetadataEncodingDetails(
      ECesiumEncodedMetadataType InType,
      ECesiumEncodedMetadataComponentType InComponentType,
      ECesiumEncodedMetadataConversion InConversion);

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

  bool operator==(const FCesiumMetadataEncodingDetails& Info) const;

  bool operator!=(const FCesiumMetadataEncodingDetails& Info) const;

  bool HasValidType() const;

  /**
   * @brief Gets the best-fitting encoded types and conversion method for a
   * given metadata type. This determines the best way (if one is possible) to
   * transfer values of the given type to the GPU, for access in Unreal
   * materials.
   *
   * An array size can also be supplied if bIsArray is true on the given value
   * type. If bIsArray is true, but the given array size is zero, this indicates
   * the arrays of the property vary in length. Variable-length array properties
   * are unsupported.
   *
   * @param PropertyDetails The metadata property details
   */
  static FCesiumMetadataEncodingDetails
  GetBestFitForProperty(const FCesiumMetadataPropertyDetails& PropertyDetails);
};
