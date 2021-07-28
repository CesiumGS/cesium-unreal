// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumMetadataProperty.h"
#include "CesiumMetadataConversions.h"

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
        return CesiumMetadataConversions<bool, decltype(value)>::convert(
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
        return CesiumMetadataConversions<uint8, decltype(value)>::convert(
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
        return CesiumMetadataConversions<int32, decltype(value)>::convert(
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
        return CesiumMetadataConversions<int64, decltype(value)>::convert(
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
        return CesiumMetadataConversions<float, decltype(value)>::convert(
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
        return CesiumMetadataConversions<FString, decltype(value)>::convert(
            value,
            defaultValue);
      },
      _property);
}

FCesiumMetadataArray FCesiumMetadataProperty::GetArray(int64 featureID) const {
  return std::visit(
      [featureID](const auto& v) -> FCesiumMetadataArray {
        auto value = v.get(featureID);
        return CesiumMetadataConversions<
            FCesiumMetadataArray,
            decltype(value)>::convert(value, FCesiumMetadataArray());
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
