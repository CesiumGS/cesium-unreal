// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumMetadataGenericValue.h"
#include "CesiumGltf/PropertyTypeTraits.h"
#include "CesiumMetadataConversions.h"

ECesiumMetadataBlueprintType
UCesiumMetadataGenericValueBlueprintLibrary::GetBlueprintType(
    UPARAM(ref) const FCesiumMetadataGenericValue& Value) {
  return CesiuMetadataTrueTypeToBlueprintType(Value._type);
}

ECesiumMetadataBlueprintType
UCesiumMetadataGenericValueBlueprintLibrary::GetBlueprintComponentType(
    UPARAM(ref) const FCesiumMetadataGenericValue& Value) {
  return CesiuMetadataTrueTypeToBlueprintType(Value._componentType);
}

ECesiumMetadataTrueType
UCesiumMetadataGenericValueBlueprintLibrary::GetTrueType(
    UPARAM(ref) const FCesiumMetadataGenericValue& Value) {
  return Value._type;
}

ECesiumMetadataTrueType
UCesiumMetadataGenericValueBlueprintLibrary::GetTrueComponentType(
    UPARAM(ref) const FCesiumMetadataGenericValue& Value) {
  return Value._componentType;
}

bool UCesiumMetadataGenericValueBlueprintLibrary::GetBoolean(
    UPARAM(ref) const FCesiumMetadataGenericValue& Value,
    bool DefaultValue) {
  return std::visit(
      [DefaultValue](auto value) -> bool {
        return CesiumMetadataConversions<bool, decltype(value)>::convert(
            value,
            DefaultValue);
      },
      Value._value);
}

uint8 UCesiumMetadataGenericValueBlueprintLibrary::GetByte(
    UPARAM(ref) const FCesiumMetadataGenericValue& Value,
    uint8 DefaultValue) {
  return std::visit(
      [DefaultValue](auto value) -> uint8 {
        return CesiumMetadataConversions<uint8, decltype(value)>::convert(
            value,
            DefaultValue);
      },
      Value._value);
}

int32 UCesiumMetadataGenericValueBlueprintLibrary::GetInteger(
    UPARAM(ref) const FCesiumMetadataGenericValue& Value,
    int32 DefaultValue) {
  return std::visit(
      [DefaultValue](auto value) {
        return CesiumMetadataConversions<int32, decltype(value)>::convert(
            value,
            DefaultValue);
      },
      Value._value);
}

int64 UCesiumMetadataGenericValueBlueprintLibrary::GetInteger64(
    UPARAM(ref) const FCesiumMetadataGenericValue& Value,
    int64 DefaultValue) {
  return std::visit(
      [DefaultValue](auto value) -> int64 {
        return CesiumMetadataConversions<int64, decltype(value)>::convert(
            value,
            DefaultValue);
      },
      Value._value);
}

float UCesiumMetadataGenericValueBlueprintLibrary::GetFloat(
    UPARAM(ref) const FCesiumMetadataGenericValue& Value,
    float DefaultValue) {
  return std::visit(
      [DefaultValue](auto value) -> float {
        return CesiumMetadataConversions<float, decltype(value)>::convert(
            value,
            DefaultValue);
      },
      Value._value);
}

FString UCesiumMetadataGenericValueBlueprintLibrary::GetString(
    UPARAM(ref) const FCesiumMetadataGenericValue& Value,
    const FString& DefaultValue) {
  return std::visit(
      [DefaultValue](auto value) -> FString {
        return CesiumMetadataConversions<FString, decltype(value)>::convert(
            value,
            DefaultValue);
      },
      Value._value);
}

FCesiumMetadataArray UCesiumMetadataGenericValueBlueprintLibrary::GetArray(
    UPARAM(ref) const FCesiumMetadataGenericValue& Value) {
  return std::visit(
      [](auto value) -> FCesiumMetadataArray {
        return CesiumMetadataConversions<
            FCesiumMetadataArray,
            decltype(value)>::convert(value, FCesiumMetadataArray());
      },
      Value._value);
}
