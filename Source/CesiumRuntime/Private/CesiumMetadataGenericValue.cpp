// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumMetadataGenericValue.h"
#include "CesiumGltf/PropertyTypeTraits.h"
#include "CesiumMetadataConversions.h"

ECesiumMetadataValueType FCesiumMetadataGenericValue::GetType() const {
  return _type;
}

bool FCesiumMetadataGenericValue::GetBoolean(bool DefaultValue) const {
  return std::visit(
      [DefaultValue](auto value) -> bool {
        return CesiumMetadataConversions<bool, decltype(value)>::convert(
            value,
            DefaultValue);
      },
      _value);
}

uint8 FCesiumMetadataGenericValue::GetByte(uint8 DefaultValue) const {
  return std::visit(
      [DefaultValue](auto value) -> uint8 {
        return CesiumMetadataConversions<uint8, decltype(value)>::convert(
            value,
            DefaultValue);
      },
      _value);
}

int32 FCesiumMetadataGenericValue::GetInteger(int32 DefaultValue) const {
  return std::visit(
      [DefaultValue](auto value) {
        return CesiumMetadataConversions<int32, decltype(value)>::convert(
            value,
            DefaultValue);
      },
      _value);
}

int64 FCesiumMetadataGenericValue::GetInteger64(int64 DefaultValue) const {
  return std::visit(
      [DefaultValue](auto value) -> int64 {
        return CesiumMetadataConversions<int64, decltype(value)>::convert(
            value,
            DefaultValue);
      },
      _value);
}

float FCesiumMetadataGenericValue::GetFloat(float DefaultValue) const {
  return std::visit(
      [DefaultValue](auto value) -> float {
        return CesiumMetadataConversions<float, decltype(value)>::convert(
            value,
            DefaultValue);
      },
      _value);
}

FString
FCesiumMetadataGenericValue::GetString(const FString& DefaultValue) const {
  return std::visit(
      [DefaultValue](auto value) -> FString {
        return CesiumMetadataConversions<FString, decltype(value)>::convert(
            value,
            DefaultValue);
      },
      _value);
}

FCesiumMetadataArray FCesiumMetadataGenericValue::GetArray() const {
  return std::visit(
      [](auto value) -> FCesiumMetadataArray {
        return CesiumMetadataConversions<
            FCesiumMetadataArray,
            decltype(value)>::convert(value, FCesiumMetadataArray());
      },
      _value);
}

ECesiumMetadataValueType UCesiumMetadataGenericValueBlueprintLibrary::GetType(
    UPARAM(ref) const FCesiumMetadataGenericValue& value) {
  return value.GetType();
}

bool UCesiumMetadataGenericValueBlueprintLibrary::GetBoolean(
    UPARAM(ref) const FCesiumMetadataGenericValue& Value,
    bool DefaultValue) {
  return Value.GetBoolean(DefaultValue);
}

uint8 UCesiumMetadataGenericValueBlueprintLibrary::GetByte(
    UPARAM(ref) const FCesiumMetadataGenericValue& Value,
    uint8 DefaultValue) {
  return Value.GetByte(DefaultValue);
}

int32 UCesiumMetadataGenericValueBlueprintLibrary::GetInteger(
    UPARAM(ref) const FCesiumMetadataGenericValue& Value,
    int32 DefaultValue) {
  return Value.GetInteger(DefaultValue);
}

int64 UCesiumMetadataGenericValueBlueprintLibrary::GetInteger64(
    UPARAM(ref) const FCesiumMetadataGenericValue& Value,
    int64 DefaultValue) {
  return Value.GetInteger64(DefaultValue);
}

float UCesiumMetadataGenericValueBlueprintLibrary::GetFloat(
    UPARAM(ref) const FCesiumMetadataGenericValue& Value,
    float DefaultValue) {
  return Value.GetFloat(DefaultValue);
}

FString UCesiumMetadataGenericValueBlueprintLibrary::GetString(
    UPARAM(ref) const FCesiumMetadataGenericValue& Value,
    const FString& DefaultValue) {
  return Value.GetString(DefaultValue);
}

FCesiumMetadataArray UCesiumMetadataGenericValueBlueprintLibrary::GetArray(
    UPARAM(ref) const FCesiumMetadataGenericValue& Value) {
  return Value.GetArray();
}
