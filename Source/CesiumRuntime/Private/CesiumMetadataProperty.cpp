// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumMetadataProperty.h"

namespace {

struct MetadataPropertySize {
  size_t operator()(std::monostate) { return 0; }

  template <typename T>
  size_t operator()(const CesiumGltf::MetadataPropertyView<T>& value) {
    return value.size();
  }
};

struct MetadataPropertyValue {
  FCesiumMetadataGenericValue operator()(std::monostate) {
    return FCesiumMetadataGenericValue{};
  }

  template <typename T>
  FCesiumMetadataGenericValue
  operator()(const CesiumGltf::MetadataPropertyView<T>& value) {
    return FCesiumMetadataGenericValue{value.get(featureID)};
  }

  size_t featureID;
};

template <typename T> struct IsNumericProperty {
  static constexpr bool value = false;
};
template <typename T>
struct IsNumericProperty<CesiumGltf::MetadataPropertyView<T>> {
  static constexpr bool value = CesiumGltf::IsMetadataNumeric<T>::value;
};

template <typename T> struct IsArrayProperty {
  static constexpr bool value = false;
};
template <typename T>
struct IsArrayProperty<CesiumGltf::MetadataPropertyView<T>> {
  static constexpr bool value = CesiumGltf::IsMetadataNumericArray<T>::value ||
                                CesiumGltf::IsMetadataBooleanArray<T>::value ||
                                CesiumGltf::IsMetadataStringArray<T>::value;
};
} // namespace

ECesiumMetadataValueType FCesiumMetadataProperty::GetType() const {
  return _type;
}

size_t FCesiumMetadataProperty::GetNumOfFeatures() const {
  return std::visit(MetadataPropertySize{}, _property);
}

int64 FCesiumMetadataProperty::GetInt64(size_t featureID) const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::Int64),
      TEXT("Value cannot be represented as Int64"));

  return std::visit(
      [featureID](const auto& v) -> int64_t {
        if constexpr (IsNumericProperty<decltype(v)>::value) {
          return static_cast<int64_t>(v.get(featureID));
        }

        assert(false && "Value cannot be represented as Int64");
        return 0;
      },
      _property);
}

uint64 FCesiumMetadataProperty::GetUint64(size_t featureID) const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::Uint64),
      TEXT("Value cannot be represented as Uint64"));

  return std::get<CesiumGltf::MetadataPropertyView<float>>(_property).get(
      featureID);
}

float FCesiumMetadataProperty::GetFloat(size_t featureID) const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::Float),
      TEXT("Value cannot be represented as Float"));

  return std::get<CesiumGltf::MetadataPropertyView<float>>(_property).get(
      featureID);
}

double FCesiumMetadataProperty::GetDouble(size_t featureID) const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::Double),
      TEXT("Value cannot be represented as Double"));

  return std::get<CesiumGltf::MetadataPropertyView<double>>(_property).get(
      featureID);
}

bool FCesiumMetadataProperty::GetBoolean(size_t featureID) const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::Boolean),
      TEXT("Value cannot be represented as Boolean"));

  return std::get<CesiumGltf::MetadataPropertyView<bool>>(_property).get(
      featureID);
}

FString FCesiumMetadataProperty::GetString(size_t featureID) const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::String),
      TEXT("Value cannot be represented as String"));

  std::string_view feature =
      std::get<CesiumGltf::MetadataPropertyView<std::string_view>>(_property)
          .get(featureID);

  return FString(feature.size(), feature.data());
}

FCesiumMetadataArray FCesiumMetadataProperty::GetArray(size_t featureID) const {
  ensureAlwaysMsgf(
      (_type == ECesiumMetadataValueType::Array),
      TEXT("Value cannot be represented as Array"));

  return std::visit(
      [featureID](const auto& value) -> FCesiumMetadataArray {
        if constexpr (IsArrayProperty<decltype(value)>::value) {
          return FCesiumMetadataArray(value.get(featureID));
        }

        assert(false && "Value cannot be represented as Array");
        return FCesiumMetadataArray();
      },
      _property);
}

FCesiumMetadataGenericValue
FCesiumMetadataProperty::GetGenericValue(size_t featureID) const {
  return std::visit(MetadataPropertyValue{featureID}, _property);
}

ECesiumMetadataValueType UCesiumMetadataPropertyBlueprintLibrary::GetType(
    UPARAM(ref) const FCesiumMetadataProperty& property) {
  return property.GetType();
}

int64 UCesiumMetadataPropertyBlueprintLibrary::GetNumOfFeatures(
    UPARAM(ref) const FCesiumMetadataProperty& property) {
  return property.GetNumOfFeatures();
}

bool UCesiumMetadataPropertyBlueprintLibrary::GetBoolean(
    UPARAM(ref) const FCesiumMetadataProperty& property,
    int64 featureID) {
  return property.GetBoolean(featureID);
}

int64 UCesiumMetadataPropertyBlueprintLibrary::GetInt64(
    UPARAM(ref) const FCesiumMetadataProperty& property,
    int64 featureID) {
  return property.GetInt64(featureID);
}

float UCesiumMetadataPropertyBlueprintLibrary::GetUint64AsFloat(
    UPARAM(ref) const FCesiumMetadataProperty& property,
    int64 featureID) {
  return static_cast<float>(property.GetUint64(featureID));
}

float UCesiumMetadataPropertyBlueprintLibrary::GetFloat(
    UPARAM(ref) const FCesiumMetadataProperty& property,
    int64 featureID) {
  return property.GetFloat(featureID);
}

float UCesiumMetadataPropertyBlueprintLibrary::GetDoubleAsFloat(
    UPARAM(ref) const FCesiumMetadataProperty& property,
    int64 featureID) {
  return static_cast<float>(property.GetDouble(featureID));
}

FString UCesiumMetadataPropertyBlueprintLibrary::GetString(
    UPARAM(ref) const FCesiumMetadataProperty& property,
    int64 featureID) {
  return property.GetString(featureID);
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
