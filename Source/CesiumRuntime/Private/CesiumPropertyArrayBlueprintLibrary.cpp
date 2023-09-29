// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumPropertyArrayBlueprintLibrary.h"
#include "CesiumGltf/PropertyTypeTraits.h"
#include "CesiumMetadataConversions.h"

ECesiumMetadataBlueprintType
UCesiumPropertyArrayBlueprintLibrary::GetElementBlueprintType(
    UPARAM(ref) const FCesiumPropertyArray& array) {
  return CesiumMetadataValueTypeToBlueprintType(array._elementType);
}

ECesiumMetadataBlueprintType
UCesiumPropertyArrayBlueprintLibrary::GetBlueprintComponentType(
    UPARAM(ref) const FCesiumPropertyArray& array) {
  return CesiumMetadataValueTypeToBlueprintType(array._elementType);
}

FCesiumMetadataValueType
UCesiumPropertyArrayBlueprintLibrary::GetElementValueType(
    UPARAM(ref) const FCesiumPropertyArray& array) {
  return array._elementType;
}

int64 UCesiumPropertyArrayBlueprintLibrary::GetArraySize(
    UPARAM(ref) const FCesiumPropertyArray& array) {
  return std::visit([](const auto& view) { return view.size(); }, array._value);
}

int64 UCesiumPropertyArrayBlueprintLibrary::GetSize(
    UPARAM(ref) const FCesiumPropertyArray& array) {
  return std::visit([](const auto& view) { return view.size(); }, array._value);
}

FCesiumMetadataValue UCesiumPropertyArrayBlueprintLibrary::GetValue(
    UPARAM(ref) const FCesiumPropertyArray& array,
    int64 index) {
  return std::visit(
      [index](const auto& v) -> FCesiumMetadataValue {
        if (index < 0 || index >= v.size()) {
          FFrame::KismetExecutionMessage(
              *FString::Printf(
                  TEXT(
                      "Attempted to access index %d from CesiumPropertyArray of length %d!"),
                  index,
                  v.size()),
              ELogVerbosity::Warning,
              FName("CesiumPropertyArrayOutOfBoundsWarning"));
          return FCesiumMetadataValue();
        }
        return FCesiumMetadataValue(v[index]);
      },
      array._value);
}

ECesiumMetadataTrueType_DEPRECATED
UCesiumPropertyArrayBlueprintLibrary::GetTrueComponentType(
    UPARAM(ref) const FCesiumPropertyArray& array) {
  return CesiumMetadataValueTypeToTrueType(array._elementType);
}

bool UCesiumPropertyArrayBlueprintLibrary::GetBoolean(
    UPARAM(ref) const FCesiumPropertyArray& array,
    int64 index,
    bool defaultValue) {
  return std::visit(
      [index, defaultValue](const auto& v) -> bool {
        if (index < 0 || index >= v.size()) {
          return defaultValue;
        }
        auto value = v[index];
        return CesiumMetadataConversions<bool, decltype(value)>::convert(
            value,
            defaultValue);
      },
      array._value);
}

uint8 UCesiumPropertyArrayBlueprintLibrary::GetByte(
    UPARAM(ref) const FCesiumPropertyArray& array,
    int64 index,
    uint8 defaultValue) {
  return std::visit(
      [index, defaultValue](const auto& v) -> uint8 {
        if (index < 0 || index >= v.size()) {
          return defaultValue;
        }
        auto value = v[index];
        return CesiumMetadataConversions<uint8, decltype(value)>::convert(
            value,
            defaultValue);
      },
      array._value);
}

int32 UCesiumPropertyArrayBlueprintLibrary::GetInteger(
    UPARAM(ref) const FCesiumPropertyArray& array,
    int64 index,
    int32 defaultValue) {
  return std::visit(
      [index, defaultValue](const auto& v) -> int32 {
        if (index < 0 || index >= v.size()) {
          return defaultValue;
        }
        auto value = v[index];
        return CesiumMetadataConversions<int32, decltype(value)>::convert(
            value,
            defaultValue);
      },
      array._value);
}

int64 UCesiumPropertyArrayBlueprintLibrary::GetInteger64(
    UPARAM(ref) const FCesiumPropertyArray& array,
    int64 index,
    int64 defaultValue) {
  return std::visit(
      [index, defaultValue](const auto& v) -> int64 {
        if (index < 0 || index >= v.size()) {
          return defaultValue;
        }
        auto value = v[index];
        return CesiumMetadataConversions<int64, decltype(value)>::convert(
            value,
            defaultValue);
      },
      array._value);
}

float UCesiumPropertyArrayBlueprintLibrary::GetFloat(
    UPARAM(ref) const FCesiumPropertyArray& array,
    int64 index,
    float defaultValue) {
  return std::visit(
      [index, defaultValue](const auto& v) -> float {
        if (index < 0 || index >= v.size()) {
          return defaultValue;
        }
        auto value = v[index];
        return CesiumMetadataConversions<float, decltype(value)>::convert(
            value,
            defaultValue);
      },
      array._value);
}

double UCesiumPropertyArrayBlueprintLibrary::GetFloat64(
    UPARAM(ref) const FCesiumPropertyArray& array,
    int64 index,
    double defaultValue) {
  return std::visit(
      [index, defaultValue](const auto& v) -> double {
        auto value = v[index];
        return CesiumMetadataConversions<double, decltype(value)>::convert(
            value,
            defaultValue);
      },
      array._value);
}

FString UCesiumPropertyArrayBlueprintLibrary::GetString(
    UPARAM(ref) const FCesiumPropertyArray& array,
    int64 index,
    const FString& defaultValue) {
  return std::visit(
      [index, defaultValue](const auto& v) -> FString {
        if (index < 0 || index >= v.size()) {
          return defaultValue;
        }
        auto value = v[index];
        return CesiumMetadataConversions<FString, decltype(value)>::convert(
            value,
            defaultValue);
      },
      array._value);
}
