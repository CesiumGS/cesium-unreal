// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumMetadataArray.h"
#include "CesiumGltf/PropertyTypeTraits.h"
#include "CesiumMetadataConversions.h"

ECesiumMetadataBlueprintType
UCesiumMetadataArrayBlueprintLibrary::GetBlueprintComponentType(
    UPARAM(ref) const FCesiumMetadataArray& array) {
  return CesiuMetadataTrueTypeToBlueprintType(array._type);
}

ECesiumMetadataTrueType
UCesiumMetadataArrayBlueprintLibrary::GetTrueComponentType(
    UPARAM(ref) const FCesiumMetadataArray& array) {
  return array._type;
}

int64 UCesiumMetadataArrayBlueprintLibrary::GetSize(
    UPARAM(ref) const FCesiumMetadataArray& array) {
  return std::visit([](const auto& view) { return view.size(); }, array._value);
}

bool UCesiumMetadataArrayBlueprintLibrary::GetBoolean(
    UPARAM(ref) const FCesiumMetadataArray& array,
    int64 index,
    bool defaultValue) {
  return std::visit(
      [index, defaultValue](const auto& v) -> bool {
        auto value = v[index];
        return CesiumMetadataConversions<bool, decltype(value)>::convert(
            value,
            defaultValue);
      },
      array._value);
}

uint8 UCesiumMetadataArrayBlueprintLibrary::GetByte(
    UPARAM(ref) const FCesiumMetadataArray& array,
    int64 index,
    uint8 defaultValue) {
  return std::visit(
      [index, defaultValue](const auto& v) -> uint8 {
        auto value = v[index];
        return CesiumMetadataConversions<uint8, decltype(value)>::convert(
            value,
            defaultValue);
      },
      array._value);
}

int32 UCesiumMetadataArrayBlueprintLibrary::GetInteger(
    UPARAM(ref) const FCesiumMetadataArray& array,
    int64 index,
    int32 defaultValue) {
  return std::visit(
      [index, defaultValue](const auto& v) -> int32 {
        auto value = v[index];
        return CesiumMetadataConversions<int32, decltype(value)>::convert(
            value,
            defaultValue);
      },
      array._value);
}

int64 UCesiumMetadataArrayBlueprintLibrary::GetInteger64(
    UPARAM(ref) const FCesiumMetadataArray& array,
    int64 index,
    int64 defaultValue) {
  return std::visit(
      [index, defaultValue](const auto& v) -> int64 {
        auto value = v[index];
        return CesiumMetadataConversions<int64, decltype(value)>::convert(
            value,
            defaultValue);
      },
      array._value);
}

float UCesiumMetadataArrayBlueprintLibrary::GetFloat(
    UPARAM(ref) const FCesiumMetadataArray& array,
    int64 index,
    float defaultValue) {
  return std::visit(
      [index, defaultValue](const auto& v) -> float {
        auto value = v[index];
        return CesiumMetadataConversions<float, decltype(value)>::convert(
            value,
            defaultValue);
      },
      array._value);
}

FString UCesiumMetadataArrayBlueprintLibrary::GetString(
    UPARAM(ref) const FCesiumMetadataArray& array,
    int64 index,
    const FString& defaultValue) {
  return std::visit(
      [index, defaultValue](const auto& v) -> FString {
        auto value = v[index];
        return CesiumMetadataConversions<FString, decltype(value)>::convert(
            value,
            defaultValue);
      },
      array._value);
}
