// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/PropertyTypeTraits.h"
#include "CesiumMetadataValueType.h"
#include "CesiumUtility/JsonValue.h"
#include <cstdlib>
#include <errno.h>
#include <glm/common.hpp>
#include <type_traits>

/**
 * Converts a FCesiumMetadataValueType to the best-fitting Blueprints type.
 *
 * @param ValueType The input metadata value type.
 */
ECesiumMetadataBlueprintType
CesiumMetadataValueTypeToBlueprintType(FCesiumMetadataValueType ValueType);

ECesiumMetadataPackedGpuType CesiumMetadataValueTypeToDefaultPackedGpuType(
    FCesiumMetadataValueType ValueType);

/**
 * Default conversion, just returns the default value.
 */
template <typename TTo, typename TFrom, typename Enable = void>
struct CesiumMetadataConversions {
  static TTo convert(TFrom from, TTo defaultValue) { return defaultValue; }
};

/**
 * Trivially converts any type to itself.
 */
template <typename T> struct CesiumMetadataConversions<T, T> {
  static T convert(T from, T defaultValue) { return from; }
};

#pragma region Conversions to boolean

/**
 * Converts from a scalar to a bool.
 */
template <typename TFrom>
struct CesiumMetadataConversions<
    bool,
    TFrom,
    std::enable_if_t<CesiumGltf::IsMetadataScalar<TFrom>::value>> {
  /**
   * Converts a scalar to a boolean. Zero is converted to false, while nonzero
   * values are converted to true.
   *
   * @param from The scalar to convert from.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static bool convert(TFrom from, bool defaultValue) {
    return from != static_cast<TFrom>(0);
  }
};

/**
 * Converts from std::string_view to a bool.
 */
template <> struct CesiumMetadataConversions<bool, std::string_view> {
  /**
   * Converts the contents of a std::string_view to a boolean.
   *
   * "0", "false", and "no" (case-insensitive) are converted to false, while
   * "1", "true", and "yes" are converted to true. All other strings will return
   * the default value.
   *
   * @param from The std::string_view to convert from.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static bool convert(const std::string_view& from, bool defaultValue) {
    FString f(from.size(), from.data());

    if (f.Compare("1", ESearchCase::IgnoreCase) == 0 ||
        f.Compare("true", ESearchCase::IgnoreCase) == 0 ||
        f.Compare("yes", ESearchCase::IgnoreCase) == 0) {
      return true;
    }

    if (f.Compare("0", ESearchCase::IgnoreCase) == 0 ||
        f.Compare("false", ESearchCase::IgnoreCase) == 0 ||
        f.Compare("no", ESearchCase::IgnoreCase) == 0) {
      return false;
    }

    return defaultValue;
  }
};

#pragma endregion

#pragma region Conversions to integer

/**
 * Converts from one integer type to another.
 */
template <typename TTo, typename TFrom>
struct CesiumMetadataConversions<
    TTo,
    TFrom,
    std::enable_if_t<
        CesiumGltf::IsMetadataInteger<TTo>::value &&
        CesiumGltf::IsMetadataInteger<TFrom>::value &&
        !std::is_same_v<TTo, TFrom>>> {
  /**
   * Converts a value of the given integer to another integer type. If the
   * integer cannot be losslessly converted to the desired type, the default
   * value is returned.
   *
   * @param from The integer value to convert from.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static TTo convert(TFrom from, TTo defaultValue) {
    return CesiumUtility::losslessNarrowOrDefault(from, defaultValue);
  }
};

/**
 * Converts from a floating-point type to an integer.
 */
template <typename TTo, typename TFrom>
struct CesiumMetadataConversions<
    TTo,
    TFrom,
    std::enable_if_t<
        CesiumGltf::IsMetadataInteger<TTo>::value &&
        CesiumGltf::IsMetadataFloating<TFrom>::value>> {
  /**
   * Converts a floating-point value to an integer type. This truncates the
   * floating-point value, rounding it towards zero.
   *
   * If the value is outside the range of the integer type, the default value is
   * returned.
   *
   * @param from The floating-point value to convert from.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static TTo convert(TFrom from, TTo defaultValue) {
    if (double(std::numeric_limits<TTo>::max()) < from ||
        double(std::numeric_limits<TTo>::lowest()) > from) {
      // Floating-point number is outside the range of this integer type.
      return defaultValue;
    }

    return static_cast<TTo>(from);
  }
};

/**
 * Converts from std::string_view to a signed integer.
 */
template <typename TTo>
struct CesiumMetadataConversions<
    TTo,
    std::string_view,
    std::enable_if_t<
        CesiumGltf::IsMetadataInteger<TTo>::value && std::is_signed_v<TTo>>> {
  /**
   * Converts the contents of a std::string_view to a signed integer. This
   * expects the entire std::string_view represents the number, not just a part
   * of it.
   *
   * This returns the default value if no number is parsed from the string.
   *
   * @param from The std::string_view to parse from.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static TTo convert(const std::string_view& from, TTo defaultValue) {
    // Amazingly, C++ has no* string parsing functions that work with strings
    // that might not be null-terminated. So we have to copy to a std::string
    // (which _is_ guaranteed to be null terminated) before parsing.
    // * except std::from_chars, but compiler/library support for the
    //   floating-point version of that method is spotty at best.
    std::string temp(from);

    char* pLastUsed;
    int64_t parsedValue = std::strtoll(temp.c_str(), &pLastUsed, 10);
    if (pLastUsed == temp.c_str() + temp.size()) {
      // Successfully parsed the entire string as an integer of this type.
      return CesiumUtility::losslessNarrowOrDefault(parsedValue, defaultValue);
    }

    // Failed to parse as an integer. Maybe we can parse as a double and
    // truncate it?
    double parsedDouble = std::strtod(temp.c_str(), &pLastUsed);
    if (pLastUsed == temp.c_str() + temp.size()) {
      // Successfully parsed the entire string as a double.
      // Convert it to an integer if we can.
      double truncated = glm::trunc(parsedDouble);

      int64_t asInteger = static_cast<int64_t>(truncated);
      double roundTrip = static_cast<double>(asInteger);
      if (roundTrip == truncated) {
        return CesiumUtility::losslessNarrowOrDefault(asInteger, defaultValue);
      }
    }

    return defaultValue;
  }
};

/**
 * Converts from std::string_view to an unsigned integer.
 */
template <typename TTo>
struct CesiumMetadataConversions<
    TTo,
    std::string_view,
    std::enable_if_t<
        CesiumGltf::IsMetadataInteger<TTo>::value && !std::is_signed_v<TTo>>> {
  /**
   * Converts the contents of a std::string_view to an signed integer. This
   * expects the entire std::string_view represents the number, not just a part
   * of it.
   *
   * This returns the default value if no number is parsed from the string.
   *
   * @param from The std::string_view to parse from.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static TTo convert(const std::string_view& from, TTo defaultValue) {
    // Amazingly, C++ has no* string parsing functions that work with strings
    // that might not be null-terminated. So we have to copy to a std::string
    // (which _is_ guaranteed to be null terminated) before parsing.
    // * except std::from_chars, but compiler/library support for the
    //   floating-point version of that method is spotty at best.
    std::string temp(from);

    char* pLastUsed;
    uint64_t parsedValue = std::strtoull(temp.c_str(), &pLastUsed, 10);
    if (pLastUsed == temp.c_str() + temp.size()) {
      // Successfully parsed the entire string as an integer of this type.
      return CesiumUtility::losslessNarrowOrDefault(parsedValue, defaultValue);
    }

    // Failed to parse as an integer. Maybe we can parse as a double and
    // truncate it?
    double parsedDouble = std::strtod(temp.c_str(), &pLastUsed);
    if (pLastUsed == temp.c_str() + temp.size()) {
      // Successfully parsed the entire string as a double.
      // Convert it to an integer if we can.
      double truncated = glm::trunc(parsedDouble);

      uint64_t asInteger = static_cast<uint64_t>(truncated);
      double roundTrip = static_cast<double>(asInteger);
      if (roundTrip == truncated) {
        return CesiumUtility::losslessNarrowOrDefault(asInteger, defaultValue);
      }
    }

    return defaultValue;
  }
};

/**
 * Converts from a boolean to an integer type.
 */
template <typename TTo>
struct CesiumMetadataConversions<
    TTo,
    bool,
    std::enable_if_t<CesiumGltf::IsMetadataInteger<TTo>::value>> {
  /**
   * Converts a boolean to an integer. This returns 1 for true, 0 for
   * false.
   *
   * @param from The boolean value to be converted.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static TTo convert(bool from, TTo defaultValue) { return from ? 1 : 0; }
};

#pragma endregion

#pragma region Conversions to float

/**
 * Converts from a boolean to a float.
 */
template <> struct CesiumMetadataConversions<float, bool> {
  /**
   * Converts a boolean to a float. This returns 1.0f for true, 0.0f for
   * false.
   *
   * @param from The boolean value to be converted.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static float convert(bool from, float defaultValue) {
    return from ? 1.0f : 0.0f;
  }
};

/**
 * Converts from an integer type to a float.
 */
template <typename TFrom>
struct CesiumMetadataConversions<
    float,
    TFrom,
    std::enable_if_t<CesiumGltf::IsMetadataInteger<TFrom>::value>> {
  /**
   * Converts an integer to a float. The value may lose precision during
   * conversion.
   *
   * @param from The integer value to be converted.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static float convert(TFrom from, float defaultValue) {
    return static_cast<float>(from);
  }
};

/**
 * Converts from a double to a float.
 */
template <> struct CesiumMetadataConversions<float, double> {
  /**
   * Converts a double to a float. The value may lose precision during
   * conversion.
   *
   * If the value is outside the range of a float, the default value is
   * returned.
   *
   * @param from The double value to be converted.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static float convert(double from, float defaultValue) {
    if (from > std::numeric_limits<float>::max() ||
        from < std::numeric_limits<float>::lowest()) {
      return defaultValue;
    }
    return static_cast<float>(from);
  }
};

/**
 * Converts from a std::string_view to a float.
 */
template <> struct CesiumMetadataConversions<float, std::string_view> {
  /**
   * Converts a std::string_view to a float. This expects the entire
   * std::string_view represents the number, not just a part of it.
   *
   * This returns the default value if no number is parsed from the string.
   *
   * @param from The std::string_view to parse from.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static float convert(const std::string_view& from, float defaultValue) {
    // Amazingly, C++ has no* string parsing functions that work with strings
    // that might not be null-terminated. So we have to copy to a std::string
    // (which _is_ guaranteed to be null terminated) before parsing.
    // * except std::from_chars, but compiler/library support for the
    //   floating-point version of that method is spotty at best.
    std::string temp(from);

    char* pLastUsed;
    float parsedValue = std::strtof(temp.c_str(), &pLastUsed);
    if (pLastUsed == temp.c_str() + temp.size() && errno == 0) {
      // Successfully parsed the entire string as a float.
      return parsedValue;
    }
    errno = 0;
    return defaultValue;
  }
};

#pragma endregion

#pragma region Conversions to double

/**
 * Converts from a boolean to a double.
 */
template <> struct CesiumMetadataConversions<double, bool> {
  /**
   * Converts a boolean to a double. This returns 1.0 for true, 0.0 for
   * false.
   *
   * @param from The boolean value to be converted.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static double convert(bool from, double defaultValue) {
    return from ? 1.0 : 0.0;
  }
};

/**
 * Converts from any integer type to a double.
 */
template <typename TFrom>
struct CesiumMetadataConversions<
    double,
    TFrom,
    std::enable_if_t<CesiumGltf::IsMetadataInteger<TFrom>::value>> {
  /**
   * Converts any integer type to a double. The value may lose precision during
   * conversion.
   *
   * @param from The boolean value to be converted.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static double convert(TFrom from, double defaultValue) {
    return static_cast<double>(from);
  }
};

/**
 * Converts from a float to a double.
 */
template <> struct CesiumMetadataConversions<double, float> {
  /**
   * Converts from a float to a double.
   *
   * @param from The float value to be converted.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static double convert(float from, double defaultValue) {
    return static_cast<double>(from);
  }
};

/**
 * Converts from std::string_view to a double.
 */
template <> struct CesiumMetadataConversions<double, std::string_view> {
  /**
   * Converts a std::string_view to a double. This expects the entire
   * std::string_view represents the number, not just a part of it.
   *
   * This returns the default value if no number is parsed from the string.
   *
   * @param from The std::string_view to parse from.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static double convert(const std::string_view& from, double defaultValue) {
    // Amazingly, C++ has no* string parsing functions that work with strings
    // that might not be null-terminated. So we have to copy to a std::string
    // (which _is_ guaranteed to be null terminated) before parsing.
    // * except std::from_chars, but compiler/library support for the
    //   floating-point version of that method is spotty at best.
    std::string temp(from);

    char* pLastUsed;
    double parsedValue = std::strtod(temp.c_str(), &pLastUsed);
    if (pLastUsed == temp.c_str() + temp.size() && errno == 0) {
      // Successfully parsed the entire string as a double.
      return parsedValue;
    }
    errno = 0;
    return defaultValue;
  }
};

#pragma endregion

#pragma region Conversions to string

/**
 * Converts from a boolean to a string.
 */
template <> struct CesiumMetadataConversions<FString, bool> {
  /**
   * Converts a boolean to a FString. Returns "true" for true and "false" for
   * false.
   *
   * @param from The boolean to be converted.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static FString convert(bool from, const FString& defaultValue) {
    return from ? "true" : "false";
  }
};

/**
 * Converts from a scalar to a string.
 */
template <typename TFrom>
struct CesiumMetadataConversions<
    FString,
    TFrom,
    std::enable_if_t<CesiumGltf::IsMetadataScalar<TFrom>::value>> {
  /**
   * Converts a scalar to a FString.
   *
   * @param from The scalar to be converted.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static FString convert(TFrom from, const FString& defaultValue) {
    return FString(std::to_string(from).c_str());
  }
};

static const std::string VectorComponents = "XYZW";

/**
 * Converts from a glm::vecN to a string.
 */
template <typename TFrom>
struct CesiumMetadataConversions<
    FString,
    TFrom,
    std::enable_if_t<CesiumGltf::IsMetadataVecN<TFrom>::value>> {
  /**
   * Converts a glm::vecN to a FString. This follows the format that ToString()
   * functions call on the Unreal vector equivalents. For example, a glm::vec3
   * will return a string in the format "X=... Y=... Z=...".
   *
   * @param from The glm::vecN to be converted.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static FString convert(const TFrom& from, const FString& defaultValue) {
    std::string result;
    for (glm::length_t i = 0; i < from.length(); i++) {
      if (i > 0) {
        result += " ";
      }
      result += VectorComponents[i] + "=" + std::to_string(from[i]);
    }
    return FString(result.c_str());
  }
};

/**
 * Converts from a glm::matN to a string.
 */
template <typename TFrom>
struct CesiumMetadataConversions<
    FString,
    TFrom,
    std::enable_if_t<CesiumGltf::IsMetadataMatN<TFrom>::value>> {
  /**
   * Converts a glm::matN to a FString. This follows the format that ToString()
   * functions call on the Unreal matrix equivalents. Each row is
   * returned in square brackets, e.g. "[1 2 3 4]", with spaces in-between.
   *
   * @param from The glm::matN to be converted.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static FString convert(const TFrom& from, const FString& defaultValue) {
    std::string result;
    glm::length_t dimensions = from.length();
    // glm::matNs are column-major, but Unreal matrices are row-major and print
    // values by row.
    for (glm::length_t c = 0; c < dimensions; c++) {
      if (c > 0) {
        result += " ";
      }
      result += "[";
      for (glm::length_t r = 0; r < dimensions; r++) {
        if (r > 0) {
          result += " ";
        }

        result += std::to_string(from[c][r]);
      }
      result += "]";
    }
    return FString(result.c_str());
  }
};

/**
 * Converts from a std::string_view to a FString.
 */
template <> struct CesiumMetadataConversions<FString, std::string_view> {
  /**
   * Converts from a std::string_view to a FString.
   */
  static FString
  convert(const std::string_view& from, const FString& defaultValue) {
    return FString(UTF8_TO_TCHAR(std::string(from.data(), from.size()).data()));
  }
};

#pragma endregion

#pragma region Conversions to integer vec3
/**
 * Converts from a boolean to a 32-bit signed integer vec3.
 */
template <> struct CesiumMetadataConversions<FIntVector, bool> {
  /**
   * Converts a boolean to a FIntVector. The boolean is converted to an integer
   * value of 1 for true or 0 for false. The returned vector is initialized with
   * this value in all of its components.
   *
   * @param from The boolean to be converted.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static FIntVector convert(bool from, const FIntVector& defaultValue) {
    int32 value = from ? 1 : 0;
    return FIntVector(value);
  }
};

/**
 * Converts from an integer type to a 32-bit signed integer vec3.
 */
template <typename TFrom>
struct CesiumMetadataConversions<
    FIntVector,
    TFrom,
    std::enable_if_t<CesiumGltf::IsMetadataInteger<TFrom>::value>> {
  /**
   * Converts an integer to a FIntVector. The returned vector is initialized
   * with the value in all of its components. If the integer cannot be
   * losslessly converted to a 32-bit signed representation, the default value
   * is returned.
   *
   * @param from The integer value to be converted.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static FIntVector convert(TFrom from, const FIntVector& defaultValue) {
    if (from > std::numeric_limits<int32_t>::max() ||
        from < std::numeric_limits<int32_t>::min()) {
      return defaultValue;
    }
    return FIntVector(static_cast<int32_t>(from));
  }
};

/**
 * Converts from a float to a 32-bit signed integer vec3.
 */
template <typename TFrom>
struct CesiumMetadataConversions<
    FIntVector,
    TFrom,
    std::enable_if_t<CesiumGltf::IsMetadataFloating<TFrom>::value>> {
  /**
   * Converts a floating-point value to a FIntVector. This truncates the
   * floating-point value, rounding it towards zero.
   *
   * If the value is outside the range that a 32-bit signed integer can
   * represent, the default value is returned.
   *
   * @param from The floating-point value to convert from.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static FIntVector convert(TFrom from, const FIntVector& defaultValue) {
    if (double(std::numeric_limits<int32_t>::max()) < from ||
        double(std::numeric_limits<int32_t>::lowest()) > from) {
      // Floating-point number is outside the range.
      return defaultValue;
    }
    return FIntVector(static_cast<int32_t>(from));
  }
};

/**
 * Converts from a glm::vec2 of any type to a 32-bit signed integer vec3.
 */
template <typename T>
struct CesiumMetadataConversions<FIntVector, glm::vec<2, T>> {
  /**
   * Converts a glm::vec2 of any type to a FIntVector. The vec2 becomes the
   * first two components of the FIntVector, while the third component is set to
   * zero. If any of the original vec2 values cannot be converted to a 32-bit
   * signed integer, the default value is returned.
   *
   * @param from The glm::vec2 to be converted.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static FIntVector
  convert(const glm::vec<2, T>& from, const FIntVector& defaultValue) {
    if constexpr (CesiumGltf::IsMetadataInteger<T>::value) {
      // Check if all values can be converted.
      for (size_t i = 0; i < 2; i++) {
        if (from[i] > std::numeric_limits<int32_t>::max() ||
            from[i] < std::numeric_limits<int32_t>::min()) {
          return defaultValue;
        }
      }
    }

    if constexpr (CesiumGltf::IsMetadataFloating<T>::value) {
      for (size_t i = 0; i < 2; i++) {
        if (from[i] > double(std::numeric_limits<int32_t>::max()) ||
            from[i] < double(std::numeric_limits<int32_t>::lowest())) {
          return defaultValue;
        }
      }
    }

    return FIntVector(
        static_cast<int32_t>(from[0]),
        static_cast<int32_t>(from[1]),
        0);
  }
};

/**
 * Converts from a glm::vec3 of any type to a 32-bit signed integer vec3.
 */
template <typename T>
struct CesiumMetadataConversions<FIntVector, glm::vec<3, T>> {
  /**
   * Converts a glm::vec3 of any type to a FIntVector. If any of the original
   * vec3 values cannot be converted to a 32-bit signed integer, the default
   * value is returned.
   *
   * @param from The glm::vec3 to be converted.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static FIntVector
  convert(const glm::vec<3, T>& from, const FIntVector& defaultValue) {
    if constexpr (CesiumGltf::IsMetadataInteger<T>::value) {
      // Check if all values can be converted.
      for (size_t i = 0; i < 3; i++) {
        if (from[i] > std::numeric_limits<int32_t>::max() ||
            from[i] < std::numeric_limits<int32_t>::min()) {
          return defaultValue;
        }
      }
    }

    if constexpr (CesiumGltf::IsMetadataFloating<T>::value) {
      for (size_t i = 0; i < 3; i++) {
        if (from[i] > double(std::numeric_limits<int32_t>::max()) ||
            from[i] < double(std::numeric_limits<int32_t>::lowest())) {
          return defaultValue;
        }
      }
    }

    return FIntVector(
        static_cast<int32_t>(from[0]),
        static_cast<int32_t>(from[1]),
        static_cast<int32_t>(from[2]));
  }
};

/**
 * Converts from a glm::vec4 of any type to a 32-bit signed integer vec3.
 */
template <typename T>
struct CesiumMetadataConversions<FIntVector, glm::vec<4, T>> {
  /**
   * Converts a glm::vec4 of any type to a FIntVector. This only uses the first
   * three components of the vec4, dropping the fourth. If any of first three
   * values cannot be converted to a 32-bit signed integer, the default value is
   * returned.
   *
   * @param from The glm::vec4 to be converted.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static FIntVector
  convert(const glm::vec<4, T>& from, const FIntVector& defaultValue) {
    if constexpr (CesiumGltf::IsMetadataInteger<T>::value) {
      // Check if all values can be converted.
      for (size_t i = 0; i < 3; i++) {
        if (from[i] > std::numeric_limits<int32_t>::max() ||
            from[i] < std::numeric_limits<int32_t>::min()) {
          return defaultValue;
        }
      }
    }

    if constexpr (CesiumGltf::IsMetadataFloating<T>::value) {
      for (size_t i = 0; i < 3; i++) {
        if (from[i] > double(std::numeric_limits<int32_t>::max()) ||
            from[i] < double(std::numeric_limits<int32_t>::lowest())) {
          return defaultValue;
        }
      }
    }

    return FIntVector(
        static_cast<int32_t>(from[0]),
        static_cast<int32_t>(from[1]),
        static_cast<int32_t>(from[2]));
  }
};

/**
 * Converts from a std::string_view to a 32-bit signed integer vec3.
 */
template <> struct CesiumMetadataConversions<FIntVector, std::string_view> {
  /**
   * Converts a std::string_view to a FIntVector. This uses
   * TVector::InitFromString, which expects the values to be written in the
   * "X=... Y=... Z=..." format. If this function fails to parse a FIntVector,
   * the default value is returned.
   *
   * @param from The std::string_view to be parsed.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static FIntVector
  convert(const std::string_view& from, const FIntVector& defaultValue) {
    FString string =
        CesiumMetadataConversions<FString, std::string_view>::convert(
            from,
            FString(""));
    UE::Math::TVector<int32_t> result;
    if (result.InitFromString(string)) {
      return FIntVector(result[0], result[1], result[2]);
    }

    return defaultValue;
  }
};

#pragma endregion

#pragma region Conversions to float vec3
/**
 * Converts from a boolean to a single-precision floating-point vec3.
 */
template <> struct CesiumMetadataConversions<FVector3f, bool> {
  /**
   * Converts a boolean to a FVector3f. The boolean is converted to a float
   * value of 1.0f for true or 0.0f for false. The returned vector is
   * initialized with this value in all of its components.
   *
   * @param from The boolean to be converted.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static FVector3f convert(bool from, const FVector3f& defaultValue) {
    float value = from ? 1.0f : 0.0f;
    return FVector3f(value);
  }
};

/**
 * Converts from an integer type to a single-precision floating-point vec3.
 */
template <typename TFrom>
struct CesiumMetadataConversions<
    FVector3f,
    TFrom,
    std::enable_if_t<CesiumGltf::IsMetadataInteger<TFrom>::value>> {
  /**
   * Converts an integer to a FVector3f. The returned vector is initialized with
   * the value in all of its components. The value may lose precision during
   * conversion.
   *
   * @param from The integer value to be converted.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static FVector3f convert(TFrom from, const FVector3f& defaultValue) {
    return FVector3f(static_cast<float>(from));
  }
};

/**
 * Converts from a float to a single-precision floating-point vec3.
 */
template <> struct CesiumMetadataConversions<FVector3f, float> {
  /**
   * Converts a float to a FVector3f. The returned vector is initialized with
   * the value in all of its components.
   *
   * @param from The float to be converted.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static FVector3f convert(float from, const FVector3f& defaultValue) {
    return FVector3f(from);
  }
};

/**
 * Converts from a double to a single-precision floating-point vec3.
 */
template <> struct CesiumMetadataConversions<FVector3f, double> {
  /**
   * Converts a double to a FVector3f. This attempts to convert the double to a
   * float, then initialize a vector with the value in all of its components. If
   * the double cannot be converted, the default value is returned.
   *
   * @param from The double to be converted.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static FVector3f convert(double from, const FVector3f& defaultValue) {
    if (from > std::numeric_limits<float>::max() ||
        from < std::numeric_limits<float>::lowest()) {
      return defaultValue;
    }
    return FVector3f(static_cast<float>(from));
  }
};

/**
 * Converts from a glm::vec3 of any type to a single-precision floating-point
 * vec3.
 */
template <glm::length_t N, typename T>
struct CesiumMetadataConversions<FVector3f, glm::vec<N, T>> {
  /**
   * Converts a glm::vecN of any type to a FVector3f. If any of the original
   * vec3 values cannot be converted to a float, the default value is
   * returned.
   *
   * If the input is a vec2, the vec2 becomes the first two components of the
   * FVector3f, while the third component is set to zero. If the input is a
   * vec4, the vec4 is truncated to its first three components.
   *
   * @param from The glm::vecN to be converted.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static FVector3f
  convert(const glm::vec<N, T>& from, const FVector3f& defaultValue) {
    if constexpr (std::is_same_v<T, double>) {
      // Check if all double values can be converted to floats.
      for (size_t i = 0; i < std::min(N, 3); i++) {
        if (from[i] > double(std::numeric_limits<float>::max()) ||
            from[i] < double(std::numeric_limits<float>::lowest())) {
          return defaultValue;
        }
      }
    }

    if constexpr (N == 2) {
      return FVector3f(
          static_cast<float>(from[0]),
          static_cast<float>(from[1]),
          0.0f);
    } else {
      return FVector3f(
          static_cast<float>(from[0]),
          static_cast<float>(from[1]),
          static_cast<float>(from[2]));
    }
  }
};

/**
 * Converts from a glm::vec4 of any type to a single-precision floating-point
 * vec3.
 */
template <typename T>
struct CesiumMetadataConversions<FVector3f, glm::vec<4, T>> {
  /**
   * Converts a glm::vec4 of any type to a FVector3f. If any of first three
   * values cannot be converted to a float, the default value is returned.
   *
   * @param from The glm::vec4 to be converted.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static FVector3f
  convert(const glm::vec<4, T>& from, const FVector3f& defaultValue) {
    if constexpr (std::is_same_v<T, double>) {
      // Check if all double values can be converted to floats.
      for (size_t i = 0; i < 3; i++) {
        if (from[i] > std::numeric_limits<float>::max() ||
            from[i] < std::numeric_limits<float>::lowest()) {
          return defaultValue;
        }
      }
    }
    return FVector3f(
        static_cast<float>(from[0]),
        static_cast<float>(from[1]),
        static_cast<float>(from[2]));
  }
};

/**
 * Converts from a std::string_view to a single-precision floating-point vec3.
 */
template <> struct CesiumMetadataConversions<FVector3f, std::string_view> {
  /**
   * Converts a std::string_view to a FVector3f. This uses
   * FVector3f::InitFromString, which expects the values to be written in the
   * "X=... Y=... Z=..." format. If this function fails to parse a FVector3f,
   * the default value is returned.
   *
   * @param from The std::string_view to be parsed.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static FVector3f
  convert(const std::string_view& from, const FVector3f& defaultValue) {
    FString string =
        CesiumMetadataConversions<FString, std::string_view>::convert(
            from,
            FString(""));
    FVector3f result;
    return result.InitFromString(string) ? result : defaultValue;
  }
};

#pragma endregion

#pragma region Conversions to double vec3
/**
 * Converts from a boolean to a double-precision floating-point vec3.
 */
template <> struct CesiumMetadataConversions<FVector, bool> {
  /**
   * Converts a boolean to a FVector. The boolean is converted to a float
   * value of 1.0 for true or 0.0 for false. The returned vector is
   * initialized with this value in all of its components.
   *
   * @param from The boolean to be converted.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static FVector convert(bool from, const FVector& defaultValue) {
    float value = from ? 1.0 : 0.0;
    return FVector(value);
  }
};

/**
 * Converts from an integer type to a double-precision floating-point vec3.
 */
template <typename TFrom>
struct CesiumMetadataConversions<
    FVector,
    TFrom,
    std::enable_if_t<CesiumGltf::IsMetadataInteger<TFrom>::value>> {
  /**
   * Converts an integer to a FVector. The returned vector is initialized with
   * the value in all of its components. The value may lose precision during
   * conversion.
   *
   * @param from The integer value to be converted.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static FVector convert(TFrom from, const FVector& defaultValue) {
    return FVector(static_cast<double>(from));
  }
};

/**
 * Converts from a float to a double-precision floating-point vec3.
 */
template <> struct CesiumMetadataConversions<FVector, float> {
  /**
   * Converts a float to a FVector. The returned vector is initialized with
   * the value in all of its components.
   *
   * @param from The float to be converted.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static FVector convert(float from, const FVector& defaultValue) {
    return FVector(static_cast<double>(from));
  }
};

/**
 * Converts from a double to a single-precision floating-point vec3.
 */
template <> struct CesiumMetadataConversions<FVector, double> {
  /**
   * Converts a double to a FVector3f. The returned vector is initialized with
   * the value in all of its components.
   *
   * @param from The double to be converted.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static FVector convert(double from, const FVector& defaultValue) {
    return FVector(from);
  }
};

/**
 * Converts from a glm::vec2 of any type to a double-precision floating-point
 * vec3.
 */
template <typename T>
struct CesiumMetadataConversions<FVector, glm::vec<2, T>> {
  /**
   * Converts a glm::vec2 of any type to a FVector3f. Similar to how an
   * FVector3f can be constructed from an FIntPoint, the vec2 becomes the first
   * two components of the FVector3f, while the third component is set to zero.
   * If the vec2 is of an integer type, its values may lose precision during
   * conversion.
   *
   * @param from The glm::vec2 to be converted.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static FVector
  convert(const glm::vec<2, T>& from, const FVector& defaultValue) {
    return FVector(
        static_cast<double>(from[0]),
        static_cast<double>(from[1]),
        0.0);
  }
};

/**
 * Converts from a glm::vec3 of any type to a double-precision floating-point
 * vec3.
 */
template <typename T>
struct CesiumMetadataConversions<FVector, glm::vec<3, T>> {
  /**
   * Converts a glm::vec3 of any type to a FVector. If the vec3 is of an integer
   * type, its values may lose precision during conversion.
   *
   * @param from The glm::vec3 to be converted.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static FVector
  convert(const glm::vec<3, T>& from, const FVector& defaultValue) {
    return FVector(
        static_cast<double>(from[0]),
        static_cast<double>(from[1]),
        static_cast<double>(from[2]));
  }
};

/**
 * Converts from a glm::vec4 of any type to a double-precision floating-point
 * vec3.
 */
template <typename T>
struct CesiumMetadataConversions<FVector, glm::vec<4, T>> {
  /**
   * Converts a glm::vec4 of any type to a FVector. This only uses the first
   * three components of the vec4, dropping the fourth. If the vec3 is of an
   * integer type, its values may lose precision during conversion.
   *
   * @param from The glm::vec4 to be converted.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static FVector
  convert(const glm::vec<4, T>& from, const FVector& defaultValue) {
    return FVector(
        static_cast<double>(from[0]),
        static_cast<double>(from[1]),
        static_cast<double>(from[2]));
  }
};

/**
 * Converts from a std::string_view to a double-precision floating-point vec3.
 */
template <> struct CesiumMetadataConversions<FVector, std::string_view> {
  /**
   * Converts a std::string_view to a FVector. This uses
   * FVector::InitFromString, which expects the values to be written in the
   * "X=... Y=... Z=..." format. If this function fails to parse a FVector,
   * the default value is returned.
   *
   * @param from The std::string_view to be parsed.
   * @param defaultValue The default value to be returned if conversion fails.
   */
  static FVector
  convert(const std::string_view& from, const FVector& defaultValue) {
    FString string =
        CesiumMetadataConversions<FString, std::string_view>::convert(
            from,
            FString(""));
    FVector result;
    return result.InitFromString(string) ? result : defaultValue;
  }
};

#pragma endregion

//#pragma region Conversions to double vec4
///**
// * Converts from a boolean to a double-precision floating-point vec4.
// */
// template <> struct CesiumMetadataConversions<FVector4, bool> {
//  /**
//   * Converts a boolean to a FVector4. The boolean is converted to a float
//   * value of 1.0f for true or 0.0f for false. The returned vector is
//   * initialized with this value in all of its components.
//   *
//   * @param from The boolean to be converted.
//   * @param defaultValue The default value to be returned if conversion fails.
//   */
//  static FVector convert(bool from, const FVector& defaultValue) {
//    float value = from ? 1.0 : 0.0;
//    return FVector(value);
//  }
//};
//
///**
// * Converts from an integer type to a double-precision floating-point vec3.
// */
// template <typename TFrom>
// struct CesiumMetadataConversions<
//    FVector,
//    TFrom,
//    std::enable_if_t<CesiumGltf::IsMetadataInteger<TFrom>::value>> {
//  /**
//   * Converts an integer to a FVector. The returned vector is initialized with
//   * the value in all of its components. The value may lose precision during
//   * conversion.
//   *
//   * @param from The integer value to be converted.
//   * @param defaultValue The default value to be returned if conversion fails.
//   */
//  static FVector convert(TFrom from, const FVector& defaultValue) {
//    return FVector(static_cast<double>(from));
//  }
//};
//
///**
// * Converts from a float to a double-precision floating-point vec3.
// */
// template <> struct CesiumMetadataConversions<FVector, float> {
//  /**
//   * Converts a float to a FVector. The returned vector is initialized with
//   * the value in all of its components.
//   *
//   * @param from The float to be converted.
//   * @param defaultValue The default value to be returned if conversion fails.
//   */
//  static FVector convert(float from, const FVector& defaultValue) {
//    return FVector(static_cast<double>(from));
//  }
//};
//
///**
// * Converts from a double to a single-precision floating-point vec3.
// */
// template <> struct CesiumMetadataConversions<FVector, double> {
//  /**
//   * Converts a double to a FVector3f. The returned vector is initialized with
//   * the value in all of its components.
//   *
//   * @param from The double to be converted.
//   * @param defaultValue The default value to be returned if conversion fails.
//   */
//  static FVector convert(double from, const FVector& defaultValue) {
//    return FVector(from);
//  }
//};
//
///**
// * Converts from a glm::vec2 of any type to a double-precision floating-point
// * vec3.
// */
// template <typename T>
// struct CesiumMetadataConversions<FVector, glm::vec<2, T>> {
//  /**
//   * Converts a glm::vec2 of any type to a FVector3f. Similar to how an
//   * FVector3f can be constructed from an FIntPoint, the vec2 becomes the
//   first
//   * two components of the FVector3f, while the third component is set to
//   zero.
//   * If the vec2 is of an integer type, its values may lose precision during
//   * conversion.
//   *
//   * @param from The glm::vec2 to be converted.
//   * @param defaultValue The default value to be returned if conversion fails.
//   */
//  static FVector
//  convert(const glm::vec<2, T>& from, const FVector& defaultValue) {
//    return FVector(
//        static_cast<double>(from[0]),
//        static_cast<double>(from[1]),
//        0.0);
//  }
//};
//
///**
// * Converts from a glm::vec3 of any type to a double-precision floating-point
// * vec3.
// */
// template <typename T>
// struct CesiumMetadataConversions<FVector, glm::vec<3, T>> {
//  /**
//   * Converts a glm::vec3 of any type to a FVector. If the vec3 is of an
//   integer
//   * type, its values may lose precision during conversion.
//   *
//   * @param from The glm::vec3 to be converted.
//   * @param defaultValue The default value to be returned if conversion fails.
//   */
//  static FVector
//  convert(const glm::vec<3, T>& from, const FVector& defaultValue) {
//    return FVector(
//        static_cast<double>(from[0]),
//        static_cast<double>(from[1]),
//        static_cast<double>(from[2]));
//  }
//};
//
///**
// * Converts from a glm::vec4 of any type to a double-precision floating-point
// * vec3.
// */
// template <typename T>
// struct CesiumMetadataConversions<FVector, glm::vec<4, T>> {
//  /**
//   * Converts a glm::vec4 of any type to a FVector. This only uses the first
//   * three components of the vec4, dropping the fourth. If the vec3 is of an
//   * integer type, its values may lose precision during conversion.
//   *
//   * @param from The glm::vec4 to be converted.
//   * @param defaultValue The default value to be returned if conversion fails.
//   */
//  static FVector
//  convert(const glm::vec<4, T>& from, const FVector& defaultValue) {
//    return FVector(
//        static_cast<double>(from[0]),
//        static_cast<double>(from[1]),
//        static_cast<double>(from[2]));
//  }
//};
//
///**
// * Converts from a std::string_view to a double-precision floating-point vec3.
// */
// template <> struct CesiumMetadataConversions<FVector, std::string_view> {
//  /**
//   * Converts a std::string_view to a FVector. This uses
//   * FVector::InitFromString, which expects the values to be written in the
//   * "X=... Y=... Z=..." format. If this function fails to parse a FVector,
//   * the default value is returned.
//   *
//   * @param from The std::string_view to be parsed.
//   * @param defaultValue The default value to be returned if conversion fails.
//   */
//  static FVector
//  convert(const std::string_view& from, const FVector& defaultValue) {
//    FString string =
//        CesiumMetadataConversions<FString, std::string_view>::convert(
//            from,
//            FString(""));
//    FVector result;
//    return result.InitFromString(string) ? result : defaultValue;
//  }
//};
//
//#pragma endregion
