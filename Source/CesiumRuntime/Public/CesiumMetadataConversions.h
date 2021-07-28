// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/PropertyTypeTraits.h"
#include "CesiumMetadataValueType.h"
#include <cstdlib>
#include <type_traits>

ECesiumMetadataBlueprintType
CesiuMetadataTrueTypeToBlueprintType(ECesiumMetadataTrueType trueType);

// Default conversion, just returns the default value.
template <typename TTo, typename TFrom, typename Enable = void>
struct CesiumMetadataConversions {
  static TTo convert(TFrom from, TTo defaultValue) { return defaultValue; }
};

// Trivially convert any type to itself.
template <typename T> struct CesiumMetadataConversions<T, T> {
  static T convert(T from, T defaultValue) { return from; }
};

//
// Conversions to boolean
//

// numeric -> bool
template <typename TFrom>
struct CesiumMetadataConversions<
    bool,
    TFrom,
    std::enable_if_t<CesiumGltf::IsMetadataNumeric<TFrom>::value>> {
  static bool convert(TFrom from, bool defaultValue) {
    return from != static_cast<TFrom>(0);
  }
};

// string -> bool
template <> struct CesiumMetadataConversions<bool, std::string_view> {
  static bool convert(const std::string_view& from, bool defaultValue) {
    FString f(from.size(), from.data());
    if (f.Compare("1", ESearchCase::IgnoreCase) == 0 ||
        f.Compare("true", ESearchCase::IgnoreCase) == 0 ||
        f.Compare("yes", ESearchCase::IgnoreCase) == 0) {
      return true;
    } else if (
        f.Compare("0", ESearchCase::IgnoreCase) == 0 ||
        f.Compare("false", ESearchCase::IgnoreCase) == 0 ||
        f.Compare("no", ESearchCase::IgnoreCase) == 0) {
      return false;
    } else {
      return defaultValue;
    }
  }
};

//
// Conversions to integer
//

// any integer -> any integer
template <typename TTo, typename TFrom>
struct CesiumMetadataConversions<
    TTo,
    TFrom,
    std::enable_if_t<
        CesiumGltf::IsMetadataInteger<TTo>::value &&
        CesiumGltf::IsMetadataInteger<TFrom>::value &&
        !std::is_same_v<TTo, TFrom>>> {
  static TTo convert(TFrom from, TTo defaultValue) {
    return CesiumUtility::losslessNarrowOrDefault(from, defaultValue);
  }
};

// float/double -> any integer
template <typename TTo, typename TFrom>
struct CesiumMetadataConversions<
    TTo,
    TFrom,
    std::enable_if_t<
        CesiumGltf::IsMetadataInteger<TTo>::value &&
        CesiumGltf::IsMetadataFloating<TFrom>::value>> {
  static TTo convert(TFrom from, TTo defaultValue) {
    if (double(std::numeric_limits<TTo>::max()) < from ||
        double(std::numeric_limits<TTo>::lowest()) > from) {
      // Floating-point number is outside the range of this integer type.
      return defaultValue;
    }

    return static_cast<TTo>(from);
  }
};

// string -> any signed integer
template <typename TTo>
struct CesiumMetadataConversions<
    TTo,
    std::string_view,
    std::enable_if_t<
        CesiumGltf::IsMetadataInteger<TTo>::value && std::is_signed_v<TTo>>> {
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

// string -> any unsigned integer
template <typename TTo>
struct CesiumMetadataConversions<
    TTo,
    std::string_view,
    std::enable_if_t<
        CesiumGltf::IsMetadataInteger<TTo>::value && !std::is_signed_v<TTo>>> {
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

// bool -> any integer
template <typename TTo>
struct CesiumMetadataConversions<
    TTo,
    bool,
    std::enable_if_t<CesiumGltf::IsMetadataInteger<TTo>::value>> {
  static TTo convert(bool from, TTo defaultValue) { return from ? 1 : 0; }
};

//
// Conversions to string
//

// bool -> string
template <> struct CesiumMetadataConversions<FString, bool> {
  static FString convert(bool from, const FString& defaultValue) {
    return from ? "true" : "false";
  }
};

// numeric -> string
template <typename TFrom>
struct CesiumMetadataConversions<
    FString,
    TFrom,
    std::enable_if_t<CesiumGltf::IsMetadataNumeric<TFrom>::value>> {
  static FString convert(TFrom from, const FString& defaultValue) {
    return FString::Format(
        TEXT("{0}"),
        FStringFormatOrderedArguments{FStringFormatArg(from)});
  }
};

// string_view -> FString
template <> struct CesiumMetadataConversions<FString, std::string_view> {
  static FString
  convert(const std::string_view& from, const FString& defaultValue) {
    return FString(from.size(), from.data());
  }
};

//
// Conversions to float
//

// bool -> float
template <> struct CesiumMetadataConversions<float, bool> {
  static float convert(bool from, float defaultValue) {
    return from ? 1.0f : 0.0f;
  }
};

// any integer -> float
template <typename TFrom>
struct CesiumMetadataConversions<
    float,
    TFrom,
    std::enable_if_t<CesiumGltf::IsMetadataInteger<TFrom>::value>> {
  static float convert(TFrom from, float defaultValue) {
    return static_cast<float>(from);
  }
};

// double -> float
template <> struct CesiumMetadataConversions<float, double> {
  static float convert(double from, float defaultValue) {
    if (from > std::numeric_limits<float>::max() ||
        from < std::numeric_limits<float>::lowest()) {
      return defaultValue;
    }
    return static_cast<float>(from);
  }
};

// string -> float
template <> struct CesiumMetadataConversions<float, std::string_view> {
  static float convert(const std::string_view& from, float defaultValue) {
    // Amazingly, C++ has no* string parsing functions that work with strings
    // that might not be null-terminated. So we have to copy to a std::string
    // (which _is_ guaranteed to be null terminated) before parsing.
    // * except std::from_chars, but compiler/library support for the
    //   floating-point version of that method is spotty at best.
    std::string temp(from);

    char* pLastUsed;
    float parsedValue = std::strtof(temp.c_str(), &pLastUsed);
    if (pLastUsed == temp.c_str() + temp.size()) {
      // Successfully parsed the entire string as a float.
      return parsedValue;
    }

    return defaultValue;
  }
};
