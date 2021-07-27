// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumMetadataProperty.h"
#include "CesiumMetadataConversions.h"
#include "CesiumUtility/JsonValue.h"
#include <charconv>
#include <optional>

using namespace CesiumGltf;

namespace {

struct MetadataPropertySize {
  size_t operator()(std::monostate) { return 0; }

  template <typename T>
  size_t operator()(const MetadataPropertyView<T>& value) {
    return value.size();
  }
};

struct MetadataPropertyValue {
  FCesiumMetadataGenericValue operator()(std::monostate) {
    return FCesiumMetadataGenericValue{};
  }

  template <typename T>
  FCesiumMetadataGenericValue operator()(const MetadataPropertyView<T>& value) {
    return FCesiumMetadataGenericValue{value.get(featureID)};
  }

  int64 featureID;
};

template <typename T> struct IsNumericProperty {
  static constexpr bool value = false;
};

template <typename T> struct IsNumericProperty<MetadataPropertyView<T>> {
  static constexpr bool value = IsMetadataNumeric<T>::value;
};

template <typename T> struct IsArrayProperty {
  static constexpr bool value = false;
};

template <typename T> struct IsArrayProperty<MetadataPropertyView<T>> {
  static constexpr bool value = IsMetadataNumericArray<T>::value ||
                                IsMetadataBooleanArray<T>::value ||
                                IsMetadataStringArray<T>::value;
};

template <typename T>
T parseStringAsFloat(const std::string_view& s, T defaultValue) {
  const char* pStart = s.data();
  const char* pEnd = s.data() + s.size();

  T parsedValue;
  std::from_chars_result result = std::from_chars(pStart, pEnd, parsedValue);
  if (result.ec == std::errc() && result.ptr == pEnd) {
    // Successfully parsed the entire string as an integer of this type.
    return parsedValue;
  }

  return defaultValue;
}

template <typename T>
T parseStringAsInteger(const std::string_view& s, T defaultValue) {
  const char* pStart = s.data();
  const char* pEnd = s.data() + s.size();

  T parsedValue;
  std::from_chars_result result =
      std::from_chars(pStart, pEnd, parsedValue, 10);
  if (result.ec == std::errc() && result.ptr == pEnd) {
    // Successfully parsed the entire string as an integer of this type.
    return parsedValue;
  }

  // Try parsing as a floating-point number instead.
  double parsedDouble;
  result = std::from_chars(pStart, pEnd, parsedDouble);
  if (result.ec == std::errc() && result.ptr == pEnd) {
    // Successfully parsed the entire string as a double of this type.
    // Convert it to an integer if we can.
    int64_t asInteger = static_cast<int64_t>(parsedDouble);
    return CesiumUtility::losslessNarrowOrDefault(asInteger, defaultValue);
  }

  return defaultValue;
}

bool equalsCaseInsensitive(const std::string_view& s1, const std::string& s2) {
  if (s1.size() != s2.size()) {
    return false;
  }

  return std::equal(s1.begin(), s1.end(), s2.begin(), [](char a, char b) {
    return std::toupper(a) == std::toupper(b);
  });
}

} // namespace

ECesiumMetadataValueType FCesiumMetadataProperty::GetType() const {
  return _type;
}

size_t FCesiumMetadataProperty::GetNumberOfFeatures() const {
  return std::visit(MetadataPropertySize{}, _property);
}

bool FCesiumMetadataProperty::GetBoolean(int64 featureID, bool defaultValue)
    const {
  return std::visit(
      [featureID, defaultValue](const auto& v) -> bool {
        auto value = v.get(featureID);
        return MetadataConverter<bool, decltype(value)>::convert(
            value,
            defaultValue);
      },
      _property);
}

uint8 FCesiumMetadataProperty::GetByte(int64 featureID, uint8 defaultValue)
    const {
  return std::visit(
      [featureID, defaultValue](const auto& v) -> uint8 {
        auto value = v.get(featureID);
        return MetadataConverter<uint8, decltype(value)>::convert(
            value,
            defaultValue);
      },
      _property);
}

int32 FCesiumMetadataProperty::GetInteger(int64 featureID, int32 defaultValue)
    const {
  return std::visit(
      [featureID, defaultValue](const auto& v) -> int32 {
        auto value = v.get(featureID);
        return MetadataConverter<int32, decltype(value)>::convert(
            value,
            defaultValue);
      },
      _property);
}

int64 FCesiumMetadataProperty::GetInteger64(int64 featureID, int64 defaultValue)
    const {
  return std::visit(
      [featureID, defaultValue](const auto& v) -> int64 {
        auto value = v.get(featureID);
        return MetadataConverter<int64, decltype(value)>::convert(
            value,
            defaultValue);
      },
      _property);
}

float FCesiumMetadataProperty::GetFloat(int64 featureID, float defaultValue)
    const {
  return std::visit(
      [featureID, defaultValue](const auto& v) -> float {
        auto value = v.get(featureID);
        return MetadataConverter<float, decltype(value)>::convert(
            value,
            defaultValue);
      },
      _property);
}

FString FCesiumMetadataProperty::GetString(
    int64 featureID,
    const FString& defaultValue) const {
  return std::visit(
      [featureID, &defaultValue](const auto& v) -> FString {
        auto value = v.get(featureID);
        return MetadataConverter<FString, decltype(value)>::convert(
            value,
            defaultValue);
      },
      _property);
}

FCesiumMetadataArray FCesiumMetadataProperty::GetArray(int64 featureID) const {
  return std::visit(
      [featureID](const auto& v) -> FCesiumMetadataArray {
        auto value = v.get(featureID);
        return MetadataConverter<FCesiumMetadataArray, decltype(value)>::
            convert(value, FCesiumMetadataArray());
      },
      _property);
}

FCesiumMetadataGenericValue
FCesiumMetadataProperty::GetGenericValue(int64 featureID) const {
  return std::visit(MetadataPropertyValue{featureID}, _property);
}

ECesiumMetadataValueType UCesiumMetadataPropertyBlueprintLibrary::GetType(
    UPARAM(ref) const FCesiumMetadataProperty& Property) {
  return Property.GetType();
}

int64 UCesiumMetadataPropertyBlueprintLibrary::GetNumberOfFeatures(
    UPARAM(ref) const FCesiumMetadataProperty& Property) {
  return Property.GetNumberOfFeatures();
}

bool UCesiumMetadataPropertyBlueprintLibrary::GetBoolean(
    UPARAM(ref) const FCesiumMetadataProperty& Property,
    int64 FeatureID,
    bool DefaultValue) {
  return Property.GetBoolean(FeatureID, DefaultValue);
}

uint8 UCesiumMetadataPropertyBlueprintLibrary::GetByte(
    UPARAM(ref) const FCesiumMetadataProperty& Property,
    int64 FeatureID,
    uint8 DefaultValue) {
  return Property.GetByte(FeatureID, DefaultValue);
}

int32 UCesiumMetadataPropertyBlueprintLibrary::GetInteger(
    UPARAM(ref) const FCesiumMetadataProperty& Property,
    int64 FeatureID,
    int32 DefaultValue) {
  return Property.GetInteger(FeatureID, DefaultValue);
}

int64 UCesiumMetadataPropertyBlueprintLibrary::GetInteger64(
    UPARAM(ref) const FCesiumMetadataProperty& Property,
    int64 FeatureID,
    int64 DefaultValue) {
  return Property.GetInteger64(FeatureID, DefaultValue);
}

float UCesiumMetadataPropertyBlueprintLibrary::GetFloat(
    UPARAM(ref) const FCesiumMetadataProperty& Property,
    int64 FeatureID,
    float DefaultValue) {
  return Property.GetFloat(FeatureID, DefaultValue);
}

FString UCesiumMetadataPropertyBlueprintLibrary::GetString(
    UPARAM(ref) const FCesiumMetadataProperty& Property,
    int64 FeatureID,
    const FString& DefaultValue) {
  return Property.GetString(FeatureID, DefaultValue);
}

FCesiumMetadataArray UCesiumMetadataPropertyBlueprintLibrary::GetArray(
    UPARAM(ref) const FCesiumMetadataProperty& property,
    int64 featureID) {
  return property.GetArray(featureID);
}

FCesiumMetadataGenericValue
UCesiumMetadataPropertyBlueprintLibrary::GetGenericValue(
    UPARAM(ref) const FCesiumMetadataProperty& property,
    int64 featureID) {
  return property.GetGenericValue(featureID);
}
