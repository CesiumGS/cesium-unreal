// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#include "CesiumPropertyArrayBlueprintLibrary.h"
#include "UnrealMetadataConversions.h"
#include <CesiumGltf/MetadataConversions.h>

ECesiumMetadataBlueprintType
UCesiumPropertyArrayBlueprintLibrary::GetElementBlueprintType(
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
  return swl::visit([](const auto& view) { return view.size(); }, array._value);
}

FCesiumMetadataValue UCesiumPropertyArrayBlueprintLibrary::GetValue(
    UPARAM(ref) const FCesiumPropertyArray& array,
    int64 index) {
  return swl::visit(
      [index, &pEnumDefinition = array._pEnumDefinition](
          const auto& v) -> FCesiumMetadataValue {
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
        return FCesiumMetadataValue(v[index], pEnumDefinition);
      },
      array._value);
}

PRAGMA_DISABLE_DEPRECATION_WARNINGS

ECesiumMetadataBlueprintType
UCesiumPropertyArrayBlueprintLibrary::GetBlueprintComponentType(
    UPARAM(ref) const FCesiumPropertyArray& array) {
  return CesiumMetadataValueTypeToBlueprintType(array._elementType);
}

ECesiumMetadataTrueType_DEPRECATED
UCesiumPropertyArrayBlueprintLibrary::GetTrueComponentType(
    UPARAM(ref) const FCesiumPropertyArray& array) {
  return CesiumMetadataValueTypeToTrueType(array._elementType);
}

int64 UCesiumPropertyArrayBlueprintLibrary::GetSize(
    UPARAM(ref) const FCesiumPropertyArray& array) {
  return swl::visit([](const auto& view) { return view.size(); }, array._value);
}

bool UCesiumPropertyArrayBlueprintLibrary::GetBoolean(
    UPARAM(ref) const FCesiumPropertyArray& array,
    int64 index,
    bool defaultValue) {
  return UCesiumMetadataValueBlueprintLibrary::GetBoolean(
      UCesiumPropertyArrayBlueprintLibrary::GetValue(array, index),
      defaultValue);
}

uint8 UCesiumPropertyArrayBlueprintLibrary::GetByte(
    UPARAM(ref) const FCesiumPropertyArray& array,
    int64 index,
    uint8 defaultValue) {
  return UCesiumMetadataValueBlueprintLibrary::GetByte(
      UCesiumPropertyArrayBlueprintLibrary::GetValue(array, index),
      defaultValue);
}

int32 UCesiumPropertyArrayBlueprintLibrary::GetInteger(
    UPARAM(ref) const FCesiumPropertyArray& array,
    int64 index,
    int32 defaultValue) {
  return UCesiumMetadataValueBlueprintLibrary::GetInteger(
      UCesiumPropertyArrayBlueprintLibrary::GetValue(array, index),
      defaultValue);
}

int64 UCesiumPropertyArrayBlueprintLibrary::GetInteger64(
    UPARAM(ref) const FCesiumPropertyArray& array,
    int64 index,
    int64 defaultValue) {
  return UCesiumMetadataValueBlueprintLibrary::GetInteger64(
      UCesiumPropertyArrayBlueprintLibrary::GetValue(array, index),
      defaultValue);
}

float UCesiumPropertyArrayBlueprintLibrary::GetFloat(
    UPARAM(ref) const FCesiumPropertyArray& array,
    int64 index,
    float defaultValue) {
  return UCesiumMetadataValueBlueprintLibrary::GetFloat(
      UCesiumPropertyArrayBlueprintLibrary::GetValue(array, index),
      defaultValue);
}

double UCesiumPropertyArrayBlueprintLibrary::GetFloat64(
    UPARAM(ref) const FCesiumPropertyArray& array,
    int64 index,
    double defaultValue) {
  return UCesiumMetadataValueBlueprintLibrary::GetFloat64(
      UCesiumPropertyArrayBlueprintLibrary::GetValue(array, index),
      defaultValue);
}

FString UCesiumPropertyArrayBlueprintLibrary::GetString(
    UPARAM(ref) const FCesiumPropertyArray& array,
    int64 index,
    const FString& defaultValue) {
  return swl::visit(
      [index, defaultValue, &pEnumDefinition = array._pEnumDefinition](
          const auto& v) -> FString {
        if (index < 0 || index >= v.size()) {
          return defaultValue;
        }
        auto value = v[index];
        using ValueType = decltype(value);

        if constexpr (CesiumGltf::IsMetadataInteger<ValueType>::value) {
          if (pEnumDefinition.IsValid()) {
            TOptional<FString> maybeName = pEnumDefinition->GetName(value);
            if (maybeName.IsSet()) {
              return maybeName.GetValue();
            }
          }
        }

        auto maybeString =
            CesiumGltf::MetadataConversions<std::string, ValueType>::convert(
                value);
        if (!maybeString) {
          return defaultValue;
        }
        return UnrealMetadataConversions::toString(*maybeString);
      },
      array._value);
}
PRAGMA_ENABLE_DEPRECATION_WARNINGS
