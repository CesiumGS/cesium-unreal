// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#pragma once
#include "CesiumMetadataValueType.h"
#include <cstdlib>
#include <type_traits>

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
  CoerceComponents,
  /**
   * Attempt to parse a color from a string property value. This supports
   * the following formats:
   * - rgb(R, G, B), where R, G, and B are values in the range [0, 255]
   * - hexcode colors, e.g. #ff0000
   */
  ParseColorFromString
};

/**
 * @brief Gets the corresponding encoded type for a given metadata component
 * type. In other words, gets the type that it will be coerced to for the GPU.
 */
ECesiumEncodedMetadataComponentType CesiumMetadataComponentTypeToEncodedType(
    ECesiumMetadataComponentType ComponentType);

/**
 * @brief Attempts to parse a color from a string property value. This supports
 * the following formats:
 * - rgb(R, G, B), where R, G, and B are values in the range [0, 255]
 * - hexcode colors, e.g. #ff0000
 *
 * @returns A TArray representing the components of the color. If a color could
 * not be parsed, the returned array will be empty.
 */
template <
    typename T,
    std::enable_if_t<std::is_same_v<T, uint8_t> || std::is_same_v<T, float>>>
static TArray<T> parseColorFromString(FString string) {
  TArray<T> result;

  // Handle hexcode case
  if (string.StartsWith(TEXT("#"))) {
    //if (string.Len() )
  }

  // Handle rgb(R, G, B) case
  if (colorString.StartsWith(TEXT("rgb(")) && colorString.EndsWith(TEXT(")"))) {
    TArray<FString> parts;
    parts.Reserve(3);
    int partCount = colorString.Mid(4, colorString.Len() - 5)
                        .ParseIntoArray(parts, TEXT(","), false);
    if (partCount == 3) {
      uint8 color[3]{255, 255, 255};
      color[0] = FCString::Atoi(*parts[0]);
      color[1] = FCString::Atoi(*parts[1]);
      color[2] = FCString::Atoi(*parts[2]);
    }
  }

  // int64 num =
  //    std::min(componentCount, std::min(int64_t(encodedFormat.pixelSize),
  //    3LL));
  // for (int64 j = 0; j < num; ++j) {
  //  *pWritePos = color[j];
  //  ++pWritePos;
  //}
  // for (int64 j = num; j < int64(encodedFormat.pixelSize); ++j) {
  //  *pWritePos = 255;
  //  ++pWritePos;
  //}

  return result;
};
