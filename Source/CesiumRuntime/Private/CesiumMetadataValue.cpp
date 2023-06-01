// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumMetadataValue.h"
#include "CesiumGltf/StructuralMetadataPropertyTypeTraits.h"
#include "CesiumMetadataConversions.h"

ECesiumMetadataBlueprintType
UCesiumMetadataValueBlueprintLibrary::GetBlueprintType(
    UPARAM(ref) const FCesiumMetadataValue& Value) {
  return CesiumMetadataTypesToBlueprintType(Value._types);
}

ECesiumMetadataBlueprintType
UCesiumMetadataValueBlueprintLibrary::GetArrayElementBlueprintType(
    UPARAM(ref) const FCesiumMetadataValue& Value) {
  if (!Value._types.bIsArray) {
    return ECesiumMetadataBlueprintType::None;
  }

  FCesiumMetadataTypes types(Value._types);
  types.bIsArray = false;

  return CesiumMetadataTypesToBlueprintType(types);
}

FCesiumMetadataTypes UCesiumMetadataValueBlueprintLibrary::GetTypes(
    UPARAM(ref) const FCesiumMetadataValue& Value) {
  return Value._types;
}

bool UCesiumMetadataValueBlueprintLibrary::GetBoolean(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    bool DefaultValue) {
  return std::visit(
      [DefaultValue](auto value) -> bool {
        return CesiumMetadataConversions<bool, decltype(value)>::convert(
            value,
            DefaultValue);
      },
      Value._value);
}

uint8 UCesiumMetadataValueBlueprintLibrary::GetByte(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    uint8 DefaultValue) {
  return std::visit(
      [DefaultValue](auto value) -> uint8 {
        return CesiumMetadataConversions<uint8, decltype(value)>::convert(
            value,
            DefaultValue);
      },
      Value._value);
}

int32 UCesiumMetadataValueBlueprintLibrary::GetInteger(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    int32 DefaultValue) {
  return std::visit(
      [DefaultValue](auto value) {
        return CesiumMetadataConversions<int32, decltype(value)>::convert(
            value,
            DefaultValue);
      },
      Value._value);
}

int64 UCesiumMetadataValueBlueprintLibrary::GetInteger64(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    int64 DefaultValue) {
  return std::visit(
      [DefaultValue](auto value) -> int64 {
        return CesiumMetadataConversions<int64, decltype(value)>::convert(
            value,
            DefaultValue);
      },
      Value._value);
}

float UCesiumMetadataValueBlueprintLibrary::GetFloat(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    float DefaultValue) {
  return std::visit(
      [DefaultValue](auto value) -> float {
        return CesiumMetadataConversions<float, decltype(value)>::convert(
            value,
            DefaultValue);
      },
      Value._value);
}

double UCesiumMetadataValueBlueprintLibrary::GetFloat64(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    double DefaultValue) {
  return std::visit(
      [DefaultValue](auto value) -> double {
        return CesiumMetadataConversions<double, decltype(value)>::convert(
            value,
            DefaultValue);
      },
      Value._value);
}

FString UCesiumMetadataValueBlueprintLibrary::GetString(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    const FString& DefaultValue) {
  return std::visit(
      [DefaultValue](auto value) -> FString {
        return CesiumMetadataConversions<FString, decltype(value)>::convert(
            value,
            DefaultValue);
      },
      Value._value);
}

FCesiumMetadataArray UCesiumMetadataValueBlueprintLibrary::GetArray(
    UPARAM(ref) const FCesiumMetadataValue& Value) {
  return std::visit(
      [](auto value) -> FCesiumMetadataArray {
        if constexpr (CesiumGltf::IsMetadataArray<decltype(value)>::value) {
          return FCesiumMetadataArray(value);
        }
        return FCesiumMetadataArray();
      },
      Value._value);
}
