// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumMetadataGenericValue.h"
#include "CesiumGltf/PropertyTypeTraits.h"

namespace {

struct GenericValueToString {
  FString operator()(std::monostate) { return ""; }

  template <typename Integer> FString operator()(Integer value) {
    return FString::FromInt(value);
  }

  FString operator()(float value) { return FString::SanitizeFloat(value); }

  FString operator()(double value) { return FString::SanitizeFloat(value); }

  FString operator()(bool value) {
    if (value) {
      return "true";
    }

    return "false";
  }

  FString operator()(std::string_view value) {
    return FString(value.size(), value.data());
  }

  template <typename T>
  FString operator()(const CesiumGltf::MetadataArrayView<T>& value) {
    FString result = "{";
    FString seperator = "";
    for (int64_t i = 0; i < value.size(); ++i) {
      result += seperator + (*this)(value[i]);
      seperator = ", ";
    }

    result += "}";
    return result;
  }
};

} // namespace

ECesiumMetadataValueType FCesiumMetadataGenericValue::GetType() const {
  return _type;
}

int64 FCesiumMetadataGenericValue::GetInt64() const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::Int64),
      TEXT("Value cannot be represented as Int64"));

  return std::visit(
      [](const auto& v) -> int64_t {
        using CoreType = std::remove_cv_t<std::remove_reference_t<decltype(v)>>;
        if constexpr (CesiumGltf::IsMetadataNumeric<CoreType>::value) {
          return static_cast<int64_t>(v);
        }

        assert(false && "Value cannot be represented as Int64");
        return 0;
      },
      _value);
}

uint64 FCesiumMetadataGenericValue::GetUint64() const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::Uint64),
      TEXT("Value cannot be represented as Uint64"));
  return std::get<uint64_t>(_value);
}

float FCesiumMetadataGenericValue::GetFloat() const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::Float),
      TEXT("Value cannot be represented as Float"));

  return std::get<float>(_value);
}

double FCesiumMetadataGenericValue::GetDouble() const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::Double),
      TEXT("Value cannot be represented as Double"));

  return std::get<double>(_value);
}

bool FCesiumMetadataGenericValue::GetBoolean() const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::Boolean),
      TEXT("Value cannot be represented as Boolean"));

  return std::get<bool>(_value);
}

FString FCesiumMetadataGenericValue::GetString() const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::String),
      TEXT("Value cannot be represented as String"));

  std::string_view val = std::get<std::string_view>(_value);
  return FString(val.size(), val.data());
}

FCesiumMetadataArray FCesiumMetadataGenericValue::GetArray() const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::Array),
      TEXT("Value cannot be represented as Array"));
  return std::visit(
      [](const auto& value) -> FCesiumMetadataArray {
        if constexpr (CesiumGltf::IsMetadataNumericArray<decltype(
                          value)>::value) {
          return FCesiumMetadataArray(value);
        }

        if constexpr (CesiumGltf::IsMetadataBooleanArray<decltype(
                          value)>::value) {
          return FCesiumMetadataArray(value);
        }

        if constexpr (CesiumGltf::IsMetadataStringArray<decltype(
                          value)>::value) {
          return FCesiumMetadataArray(value);
        }

        assert(false && "Value cannot be represented as Array");
        return FCesiumMetadataArray();
      },
      _value);
}

FString FCesiumMetadataGenericValue::ToString() const {
  return std::visit(GenericValueToString{}, _value);
}

ECesiumMetadataValueType UCesiumMetadataGenericValueBlueprintLibrary::GetType(
    UPARAM(ref) const FCesiumMetadataGenericValue& value) {
  return value.GetType();
}

int64 UCesiumMetadataGenericValueBlueprintLibrary::GetInt64(
    UPARAM(ref) const FCesiumMetadataGenericValue& value) {
  return value.GetInt64();
}

float UCesiumMetadataGenericValueBlueprintLibrary::GetUint64AsFloat(
    UPARAM(ref) const FCesiumMetadataGenericValue& value) {
  return static_cast<float>(value.GetUint64());
}

float UCesiumMetadataGenericValueBlueprintLibrary::GetFloat(
    UPARAM(ref) const FCesiumMetadataGenericValue& value) {
  return value.GetFloat();
}

float UCesiumMetadataGenericValueBlueprintLibrary::GetDoubleAsFloat(
    UPARAM(ref) const FCesiumMetadataGenericValue& value) {
  return value.GetDouble();
}

bool UCesiumMetadataGenericValueBlueprintLibrary::GetBoolean(
    UPARAM(ref) const FCesiumMetadataGenericValue& value) {
  return value.GetBoolean();
}

FString UCesiumMetadataGenericValueBlueprintLibrary::GetString(
    UPARAM(ref) const FCesiumMetadataGenericValue& value) {
  return value.GetString();
}

FCesiumMetadataArray UCesiumMetadataGenericValueBlueprintLibrary::GetArray(
    UPARAM(ref) const FCesiumMetadataGenericValue& value) {
  return value.GetArray();
}

FString UCesiumMetadataGenericValueBlueprintLibrary::ToString(
    UPARAM(ref) const FCesiumMetadataGenericValue& value) {
  return value.ToString();
}
