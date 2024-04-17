// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/PropertyTypeTraits.h"
#include "CesiumMetadataValueType.h"
#include "CesiumUtility/JsonValue.h"
#include <cstdlib>
#include <glm/common.hpp>
#include <type_traits>

enum class ECesiumEncodedMetadataComponentType : uint8;

/**
 * @brief Converts a FCesiumMetadataValueType to the best-fitting Blueprints
 * type.
 *
 * @param ValueType The input metadata value type.
 */
ECesiumMetadataBlueprintType
CesiumMetadataValueTypeToBlueprintType(FCesiumMetadataValueType ValueType);

// Deprecated.
ECesiumMetadataBlueprintType CesiumMetadataTrueTypeToBlueprintType(
    ECesiumMetadataTrueType_DEPRECATED trueType);

// For backwards compatibility.
ECesiumMetadataTrueType_DEPRECATED
CesiumMetadataValueTypeToTrueType(FCesiumMetadataValueType ValueType);

static const std::string VectorComponents = "XYZW";

struct UnrealMetadataConversions {
public:
  static FIntPoint toIntPoint(const glm::ivec2& vec2);
  /**
   * Converts a std::string_view to a FIntPoint. This expects the values to be
   * written in the "X=... Y=..." format. If this function fails to parse
   * a FIntPoint, the default value is returned.
   */
  static FIntPoint
  toIntPoint(const std::string_view& string, const FIntPoint& defaultValue);

  static FVector2D toVector2D(const glm::dvec2& vec2);
  /**
   * Converts a std::string_view to a FVector2D. This uses
   * FVector2D::InitFromString, which expects the values to be written in the
   * "X=... Y=..." format. If this function fails to parse a FVector2D, the
   * default value is returned.
   */
  static FVector2D
  toVector2D(const std::string_view& string, const FVector2D& defaultValue);

  static FIntVector toIntVector(const glm::ivec3& vec3);
  /**
   * Converts a std::string_view to a FIntVector. This expects the values to be
   * written in the "X=... Y=... Z=..." format. If this function fails to parse
   * a FIntVector, the default value is returned.
   *
   * @param from The std::string_view to be parsed.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static FIntVector
  toIntVector(const std::string_view& string, const FIntVector& defaultValue);

  static FVector3f toVector3f(const glm::vec3& vec3);
  static FVector3f
  toVector3f(const std::string_view& string, const FVector3f& defaultValue);

  static FVector toVector(const glm::dvec3& vec3);
  static FVector
  toVector(const std::string_view& string, const FVector& defaultValue);

  static FVector4 toVector4(const glm::dvec4& vec4);
  static FVector4
  toVector4(const std::string_view& string, const FVector4& defaultValue);

  static FMatrix toMatrix(const glm::dmat4& mat4);

  /**
   * Converts a glm::vecN to a FString. This follows the format that ToString()
   * functions call on the Unreal vector equivalents. For example, a glm::vec3
   * will return a string in the format "X=... Y=... Z=...".
   *
   * @param from The glm::vecN to be converted.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  template <glm::length_t N, typename T>
  static FString toString(const glm::vec<N, T>& from) {
    std::string result;
    for (glm::length_t i = 0; i < N; i++) {
      if (i > 0) {
        result += " ";
      }
      result += VectorComponents[i];
      result += "=";
      result += std::to_string(from[i]);
    }
    return FString(result.c_str());
  }

  /**
   * Converts a glm::matN to a FString. This follows the format that ToString()
   * functions call on the Unreal matrix equivalents. Each row is
   * returned in square brackets, e.g. "[1 2 3 4]", with spaces in-between.
   *
   * @param from The glm::matN to be converted.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  template <glm::length_t N, typename T>

  static FString toString(const glm::mat<N, N, T>& from) {
    std::string result;
    // glm::matNs are column-major, but Unreal matrices are row-major and print
    // their values by row.
    for (glm::length_t r = 0; r < N; r++) {
      if (r > 0) {
        result += " ";
      }
      result += "[";
      for (glm::length_t c = 0; c < N; c++) {
        if (c > 0) {
          result += " ";
        }
        result += std::to_string(from[c][r]);
      }
      result += "]";
    }
    return FString(result.c_str());
  }

  static FString toString(const std::string_view& from);

  static FString toString(const std::string& from);
};
