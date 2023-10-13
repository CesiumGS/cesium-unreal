// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumMetadataConversions.h"

ECesiumMetadataBlueprintType
CesiumMetadataValueTypeToBlueprintType(FCesiumMetadataValueType ValueType) {
  if (ValueType.bIsArray) {
    return ECesiumMetadataBlueprintType::Array;
  }

  ECesiumMetadataType type = ValueType.Type;
  ECesiumMetadataComponentType componentType = ValueType.ComponentType;

  if (type == ECesiumMetadataType::Boolean) {
    return ECesiumMetadataBlueprintType::Boolean;
  }

  if (type == ECesiumMetadataType::String) {
    return ECesiumMetadataBlueprintType::String;
  }

  if (type == ECesiumMetadataType::Scalar) {
    switch (componentType) {
    case ECesiumMetadataComponentType::Uint8:
      return ECesiumMetadataBlueprintType::Byte;
    case ECesiumMetadataComponentType::Int8:
    case ECesiumMetadataComponentType::Int16:
    case ECesiumMetadataComponentType::Uint16:
    case ECesiumMetadataComponentType::Int32:
      return ECesiumMetadataBlueprintType::Integer;
    case ECesiumMetadataComponentType::Uint32:
    case ECesiumMetadataComponentType::Int64:
      return ECesiumMetadataBlueprintType::Integer64;
    case ECesiumMetadataComponentType::Float32:
      return ECesiumMetadataBlueprintType::Float;
    case ECesiumMetadataComponentType::Float64:
      return ECesiumMetadataBlueprintType::Float64;
    case ECesiumMetadataComponentType::Uint64:
    default:
      return ECesiumMetadataBlueprintType::String;
    }
  }

  if (type == ECesiumMetadataType::Vec2) {
    switch (componentType) {
    case ECesiumMetadataComponentType::Uint8:
    case ECesiumMetadataComponentType::Int8:
    case ECesiumMetadataComponentType::Int16:
    case ECesiumMetadataComponentType::Uint16:
    case ECesiumMetadataComponentType::Int32:
      return ECesiumMetadataBlueprintType::IntPoint;
    default:
      return ECesiumMetadataBlueprintType::Vector2D;
    }
  }

  if (type == ECesiumMetadataType::Vec3) {
    switch (componentType) {
    case ECesiumMetadataComponentType::Uint8:
    case ECesiumMetadataComponentType::Int8:
    case ECesiumMetadataComponentType::Int16:
    case ECesiumMetadataComponentType::Uint16:
    case ECesiumMetadataComponentType::Int32:
      return ECesiumMetadataBlueprintType::IntVector;
    case ECesiumMetadataComponentType::Float32:
      return ECesiumMetadataBlueprintType::Vector3f;
    default:
      return ECesiumMetadataBlueprintType::Vector3;
    }
  }

  if (type == ECesiumMetadataType::Vec4) {
    return ECesiumMetadataBlueprintType::Vector4;
  }

  if (type == ECesiumMetadataType::Mat2 || type == ECesiumMetadataType::Mat3 ||
      type == ECesiumMetadataType::Mat4) {
    return ECesiumMetadataBlueprintType::Matrix;
  }

  return ECesiumMetadataBlueprintType::None;
}

ECesiumMetadataBlueprintType CesiumMetadataTrueTypeToBlueprintType(
    ECesiumMetadataTrueType_DEPRECATED trueType) {
  switch (trueType) {
  case ECesiumMetadataTrueType_DEPRECATED::Boolean_DEPRECATED:
    return ECesiumMetadataBlueprintType::Boolean;
  case ECesiumMetadataTrueType_DEPRECATED::Uint8_DEPRECATED:
    return ECesiumMetadataBlueprintType::Byte;
  case ECesiumMetadataTrueType_DEPRECATED::Int8_DEPRECATED:
  case ECesiumMetadataTrueType_DEPRECATED::Int16_DEPRECATED:
  case ECesiumMetadataTrueType_DEPRECATED::Uint16_DEPRECATED:
  case ECesiumMetadataTrueType_DEPRECATED::Int32_DEPRECATED:
  // TODO: remove this one
  case ECesiumMetadataTrueType_DEPRECATED::Uint32_DEPRECATED:
    return ECesiumMetadataBlueprintType::Integer;
  case ECesiumMetadataTrueType_DEPRECATED::Int64_DEPRECATED:
    return ECesiumMetadataBlueprintType::Integer64;
  case ECesiumMetadataTrueType_DEPRECATED::Float32_DEPRECATED:
    return ECesiumMetadataBlueprintType::Float;
  case ECesiumMetadataTrueType_DEPRECATED::Float64_DEPRECATED:
    return ECesiumMetadataBlueprintType::Float64;
  case ECesiumMetadataTrueType_DEPRECATED::Uint64_DEPRECATED:
  case ECesiumMetadataTrueType_DEPRECATED::String_DEPRECATED:
    return ECesiumMetadataBlueprintType::String;
  case ECesiumMetadataTrueType_DEPRECATED::Array_DEPRECATED:
    return ECesiumMetadataBlueprintType::Array;
  default:
    return ECesiumMetadataBlueprintType::None;
  }
}

ECesiumMetadataTrueType_DEPRECATED
CesiumMetadataValueTypeToTrueType(FCesiumMetadataValueType ValueType) {
  if (ValueType.bIsArray) {
    return ECesiumMetadataTrueType_DEPRECATED::Array_DEPRECATED;
  }

  CesiumGltf::PropertyType type = CesiumGltf::PropertyType(ValueType.Type);
  CesiumGltf::PropertyComponentType componentType =
      CesiumGltf::PropertyComponentType(ValueType.ComponentType);

  if (type == CesiumGltf::PropertyType::Boolean) {
    return ECesiumMetadataTrueType_DEPRECATED::Boolean_DEPRECATED;
  }

  if (type == CesiumGltf::PropertyType::Scalar) {
    switch (componentType) {
    case CesiumGltf::PropertyComponentType::Uint8:
      return ECesiumMetadataTrueType_DEPRECATED::Uint8_DEPRECATED;
    case CesiumGltf::PropertyComponentType::Int8:
      return ECesiumMetadataTrueType_DEPRECATED::Int8_DEPRECATED;
    case CesiumGltf::PropertyComponentType::Uint16:
      return ECesiumMetadataTrueType_DEPRECATED::Uint16_DEPRECATED;
    case CesiumGltf::PropertyComponentType::Int16:
      return ECesiumMetadataTrueType_DEPRECATED::Int16_DEPRECATED;
    case CesiumGltf::PropertyComponentType::Uint32:
      return ECesiumMetadataTrueType_DEPRECATED::Uint32_DEPRECATED;
    case CesiumGltf::PropertyComponentType::Int32:
      return ECesiumMetadataTrueType_DEPRECATED::Int32_DEPRECATED;
    case CesiumGltf::PropertyComponentType::Int64:
      return ECesiumMetadataTrueType_DEPRECATED::Int64_DEPRECATED;
    case CesiumGltf::PropertyComponentType::Uint64:
      return ECesiumMetadataTrueType_DEPRECATED::Uint64_DEPRECATED;
    case CesiumGltf::PropertyComponentType::Float32:
      return ECesiumMetadataTrueType_DEPRECATED::Float32_DEPRECATED;
    case CesiumGltf::PropertyComponentType::Float64:
      return ECesiumMetadataTrueType_DEPRECATED::Float64_DEPRECATED;
    default:
      return ECesiumMetadataTrueType_DEPRECATED::None_DEPRECATED;
    }
  }

  if (type == CesiumGltf::PropertyType::String) {
    return ECesiumMetadataTrueType_DEPRECATED::String_DEPRECATED;
  }

  return ECesiumMetadataTrueType_DEPRECATED::None_DEPRECATED;
}

//bool CesiumMetadataConversions<bool, std::string_view>::convert(
//    const std::string_view& from,
//    bool defaultValue) noexcept {
//  FString f(from.size(), from.data());
//
//  if (f.Compare("1", ESearchCase::IgnoreCase) == 0 ||
//      f.Compare("true", ESearchCase::IgnoreCase) == 0 ||
//      f.Compare("yes", ESearchCase::IgnoreCase) == 0) {
//    return true;
//  }
//
//  if (f.Compare("0", ESearchCase::IgnoreCase) == 0 ||
//      f.Compare("false", ESearchCase::IgnoreCase) == 0 ||
//      f.Compare("no", ESearchCase::IgnoreCase) == 0) {
//    return false;
//  }
//
//  return defaultValue;
//}
//
//template <typename TTo, typename TFrom>
//TTo CesiumMetadataConversions<
//    TTo,
//    TFrom,
//    std::enable_if_t<
//        CesiumGltf::IsMetadataInteger<TTo>::value &&
//        CesiumGltf::IsMetadataInteger<TFrom>::value &&
//        !std::is_same_v<TTo, TFrom>>>::
//    convert(TFrom from, TTo defaultValue) noexcept {
//  return CesiumUtility::losslessNarrowOrDefault(from, defaultValue);
//}
//
//template struct CesiumMetadataConversions<int8_t, uint8_t, void>;
//template struct CesiumMetadataConversions<int8_t, int16_t, void>;
//template struct CesiumMetadataConversions<int8_t, uint16_t, void>;
//template struct CesiumMetadataConversions<int8_t, int32_t, void>;
//template struct CesiumMetadataConversions<int8_t, uint32_t, void>;
//template struct CesiumMetadataConversions<int8_t, int64_t, void>;
//template struct CesiumMetadataConversions<int8_t, uint64_t, void>;
//
//template struct CesiumMetadataConversions<uint8_t, int8_t, void>;
//template struct CesiumMetadataConversions<uint8_t, int16_t, void>;
//template struct CesiumMetadataConversions<uint8_t, uint16_t, void>;
//template struct CesiumMetadataConversions<uint8_t, int32_t, void>;
//template struct CesiumMetadataConversions<uint8_t, uint32_t, void>;
//template struct CesiumMetadataConversions<uint8_t, int64_t, void>;
//template struct CesiumMetadataConversions<uint8_t, uint64_t, void>;
//
//template struct CesiumMetadataConversions<int16_t, int8_t, void>;
//template struct CesiumMetadataConversions<int16_t, uint8_t, void>;
//template struct CesiumMetadataConversions<int16_t, uint16_t, void>;
//template struct CesiumMetadataConversions<int16_t, int32_t, void>;
//template struct CesiumMetadataConversions<int16_t, uint32_t, void>;
//template struct CesiumMetadataConversions<int16_t, int64_t, void>;
//template struct CesiumMetadataConversions<int16_t, uint64_t, void>;
//
//template struct CesiumMetadataConversions<uint16_t, int8_t, void>;
//template struct CesiumMetadataConversions<uint16_t, uint8_t, void>;
//template struct CesiumMetadataConversions<uint16_t, int16_t, void>;
//template struct CesiumMetadataConversions<uint16_t, int32_t, void>;
//template struct CesiumMetadataConversions<uint16_t, uint32_t, void>;
//template struct CesiumMetadataConversions<uint16_t, int64_t, void>;
//template struct CesiumMetadataConversions<uint16_t, uint64_t, void>;
//
//template struct CesiumMetadataConversions<int32_t, int8_t, void>;
//template struct CesiumMetadataConversions<int32_t, uint8_t, void>;
//template struct CesiumMetadataConversions<int32_t, int16_t, void>;
//template struct CesiumMetadataConversions<int32_t, uint16_t, void>;
//template struct CesiumMetadataConversions<int32_t, uint32_t, void>;
//template struct CesiumMetadataConversions<int32_t, int64_t, void>;
//template struct CesiumMetadataConversions<int32_t, uint64_t, void>;
//
//template struct CesiumMetadataConversions<uint32_t, int8_t, void>;
//template struct CesiumMetadataConversions<uint32_t, uint8_t, void>;
//template struct CesiumMetadataConversions<uint32_t, int16_t, void>;
//template struct CesiumMetadataConversions<uint32_t, uint16_t, void>;
//template struct CesiumMetadataConversions<uint32_t, int32_t, void>;
//template struct CesiumMetadataConversions<uint32_t, int64_t, void>;
//template struct CesiumMetadataConversions<uint32_t, uint64_t, void>;
//
//template struct CesiumMetadataConversions<int64_t, int8_t, void>;
//template struct CesiumMetadataConversions<int64_t, uint8_t, void>;
//template struct CesiumMetadataConversions<int64_t, int16_t, void>;
//template struct CesiumMetadataConversions<int64_t, uint16_t, void>;
//template struct CesiumMetadataConversions<int64_t, int32_t, void>;
//template struct CesiumMetadataConversions<int64_t, uint32_t, void>;
//template struct CesiumMetadataConversions<int64_t, uint64_t, void>;
//
//template struct CesiumMetadataConversions<uint64_t, int8_t, void>;
//template struct CesiumMetadataConversions<uint64_t, uint8_t, void>;
//template struct CesiumMetadataConversions<uint64_t, int16_t, void>;
//template struct CesiumMetadataConversions<uint64_t, uint16_t, void>;
//template struct CesiumMetadataConversions<uint64_t, int32_t, void>;
//template struct CesiumMetadataConversions<uint64_t, uint32_t, void>;
//template struct CesiumMetadataConversions<uint64_t, int64_t, void>;
//
//template <typename TTo, typename TFrom>
//TTo CesiumMetadataConversions<
//    TTo,
//    TFrom,
//    std::enable_if_t<
//        CesiumGltf::IsMetadataInteger<TTo>::value &&
//        CesiumGltf::IsMetadataFloating<TFrom>::value>>::
//    convert(TFrom from, TTo defaultValue) noexcept {
//  if (double(std::numeric_limits<TTo>::max()) < from ||
//      double(std::numeric_limits<TTo>::lowest()) > from) {
//    // Floating-point number is outside the range of this integer type.
//    return defaultValue;
//  }
//
//  return static_cast<TTo>(from);
//}
//
//template struct CesiumMetadataConversions<int8_t, float, void>;
//template struct CesiumMetadataConversions<uint8_t, float, void>;
//template struct CesiumMetadataConversions<int16_t, float, void>;
//template struct CesiumMetadataConversions<uint16_t, float, void>;
//template struct CesiumMetadataConversions<int32_t, float, void>;
//template struct CesiumMetadataConversions<uint32_t, float, void>;
//template struct CesiumMetadataConversions<int64_t, float, void>;
//template struct CesiumMetadataConversions<uint64_t, float, void>;
//
//template struct CesiumMetadataConversions<int8_t, double, void>;
//template struct CesiumMetadataConversions<uint8_t, double, void>;
//template struct CesiumMetadataConversions<int16_t, double, void>;
//template struct CesiumMetadataConversions<uint16_t, double, void>;
//template struct CesiumMetadataConversions<int32_t, double, void>;
//template struct CesiumMetadataConversions<uint32_t, double, void>;
//template struct CesiumMetadataConversions<int64_t, double, void>;
//template struct CesiumMetadataConversions<uint64_t, double, void>;
//
//template <typename TTo>
//TTo CesiumMetadataConversions<
//    TTo,
//    std::string_view,
//    std::enable_if_t<
//        CesiumGltf::IsMetadataInteger<TTo>::value && std::is_signed_v<TTo>>>::
//    convert(const std::string_view& from, TTo defaultValue) noexcept {
//  // Amazingly, C++ has no* string parsing functions that work with strings
//  // that might not be null-terminated. So we have to copy to a std::string
//  // (which _is_ guaranteed to be null terminated) before parsing.
//  // * except std::from_chars, but compiler/library support for the
//  //   floating-point version of that method is spotty at best.
//  std::string temp(from);
//
//  char* pLastUsed;
//  int64_t parsedValue = std::strtoll(temp.c_str(), &pLastUsed, 10);
//  if (pLastUsed == temp.c_str() + temp.size()) {
//    // Successfully parsed the entire string as an integer of this type.
//    return CesiumUtility::losslessNarrowOrDefault(parsedValue, defaultValue);
//  }
//
//  // Failed to parse as an integer. Maybe we can parse as a double and
//  // truncate it?
//  double parsedDouble = std::strtod(temp.c_str(), &pLastUsed);
//  if (pLastUsed == temp.c_str() + temp.size()) {
//    // Successfully parsed the entire string as a double.
//    // Convert it to an integer if we can.
//    double truncated = glm::trunc(parsedDouble);
//
//    int64_t asInteger = static_cast<int64_t>(truncated);
//    double roundTrip = static_cast<double>(asInteger);
//    if (roundTrip == truncated) {
//      return CesiumUtility::losslessNarrowOrDefault(asInteger, defaultValue);
//    }
//  }
//
//  return defaultValue;
//}
//
//template struct CesiumMetadataConversions<int8_t, std::string_view, void>;
//template struct CesiumMetadataConversions<int16_t, std::string_view, void>;
//template struct CesiumMetadataConversions<int32_t, std::string_view, void>;
//template struct CesiumMetadataConversions<int64_t, std::string_view, void>;
//
//template <typename TTo>
//TTo CesiumMetadataConversions<
//    TTo,
//    std::string_view,
//    std::enable_if_t<
//        CesiumGltf::IsMetadataInteger<TTo>::value && !std::is_signed_v<TTo>>>::
//    convert(const std::string_view& from, TTo defaultValue) noexcept {
//  // Amazingly, C++ has no* string parsing functions that work with strings
//  // that might not be null-terminated. So we have to copy to a std::string
//  // (which _is_ guaranteed to be null terminated) before parsing.
//  // * except std::from_chars, but compiler/library support for the
//  //   floating-point version of that method is spotty at best.
//  std::string temp(from);
//
//  char* pLastUsed;
//  uint64_t parsedValue = std::strtoull(temp.c_str(), &pLastUsed, 10);
//  if (pLastUsed == temp.c_str() + temp.size()) {
//    // Successfully parsed the entire string as an integer of this type.
//    return CesiumUtility::losslessNarrowOrDefault(parsedValue, defaultValue);
//  }
//
//  // Failed to parse as an integer. Maybe we can parse as a double and
//  // truncate it?
//  double parsedDouble = std::strtod(temp.c_str(), &pLastUsed);
//  if (pLastUsed == temp.c_str() + temp.size()) {
//    // Successfully parsed the entire string as a double.
//    // Convert it to an integer if we can.
//    double truncated = glm::trunc(parsedDouble);
//
//    uint64_t asInteger = static_cast<uint64_t>(truncated);
//    double roundTrip = static_cast<double>(asInteger);
//    if (roundTrip == truncated) {
//      return CesiumUtility::losslessNarrowOrDefault(asInteger, defaultValue);
//    }
//  }
//
//  return defaultValue;
//}
//
//template struct CesiumMetadataConversions<uint8_t, std::string_view, void>;
//template struct CesiumMetadataConversions<uint16_t, std::string_view, void>;
//template struct CesiumMetadataConversions<uint32_t, std::string_view, void>;
//template struct CesiumMetadataConversions<uint64_t, std::string_view, void>;
//
//float CesiumMetadataConversions<float, std::string_view>::convert(
//    const std::string_view& from,
//    float defaultValue) noexcept {
//  // Amazingly, C++ has no* string parsing functions that work with strings
//  // that might not be null-terminated. So we have to copy to a std::string
//  // (which _is_ guaranteed to be null terminated) before parsing.
//  // * except std::from_chars, but compiler/library support for the
//  //   floating-point version of that method is spotty at best.
//  std::string temp(from);
//
//  char* pLastUsed;
//  float parsedValue = std::strtof(temp.c_str(), &pLastUsed);
//  if (pLastUsed == temp.c_str() + temp.size() && !std::isinf(parsedValue)) {
//    // Successfully parsed the entire string as a float.
//    return parsedValue;
//  }
//  return defaultValue;
//}
//
//double CesiumMetadataConversions<double, std::string_view>::convert(
//    const std::string_view& from,
//    double defaultValue) noexcept {
//  // Amazingly, C++ has no* string parsing functions that work with strings
//  // that might not be null-terminated. So we have to copy to a std::string
//  // (which _is_ guaranteed to be null terminated) before parsing.
//  // * except std::from_chars, but compiler/library support for the
//  //   floating-point version of that method is spotty at best.
//  std::string temp(from);
//
//  char* pLastUsed;
//  double parsedValue = std::strtod(temp.c_str(), &pLastUsed);
//  if (pLastUsed == temp.c_str() + temp.size() && !std::isinf(parsedValue)) {
//    // Successfully parsed the entire string as a double.
//    return parsedValue;
//  }
//  return defaultValue;
//}
//
//FString CesiumMetadataConversions<FString, bool>::convert(
//    bool from,
//    const FString& defaultValue) noexcept {
//  return from ? "true" : "false";
//}
//
//template <typename TFrom>
//FString CesiumMetadataConversions<
//    FString,
//    TFrom,
//    std::enable_if_t<CesiumGltf::IsMetadataScalar<TFrom>::value>>::
//    convert(TFrom from, const FString& defaultValue) noexcept {
//  return FString(std::to_string(from).c_str());
//}
//
//template struct CesiumMetadataConversions<FString, int8_t, void>;
//template struct CesiumMetadataConversions<FString, uint8_t, void>;
//template struct CesiumMetadataConversions<FString, int16_t, void>;
//template struct CesiumMetadataConversions<FString, uint16_t, void>;
//template struct CesiumMetadataConversions<FString, int32_t, void>;
//template struct CesiumMetadataConversions<FString, uint32_t, void>;
//template struct CesiumMetadataConversions<FString, int64_t, void>;
//template struct CesiumMetadataConversions<FString, uint64_t, void>;
//template struct CesiumMetadataConversions<FString, float, void>;
//template struct CesiumMetadataConversions<FString, double, void>;
