// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumPropertyArrayBlueprintLibrary.h"
#include "UnrealMetadataConversions.h"
#include <CesiumGltf/MetadataConversions.h>

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
  return swl::visit([](const auto& view) { return view.size(); }, array._value);
}

int64 UCesiumPropertyArrayBlueprintLibrary::GetSize(
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

ECesiumMetadataTrueType_DEPRECATED
UCesiumPropertyArrayBlueprintLibrary::GetTrueComponentType(
    UPARAM(ref) const FCesiumPropertyArray& array) {
  return CesiumMetadataValueTypeToTrueType(array._elementType);
}

bool UCesiumPropertyArrayBlueprintLibrary::GetBoolean(
    UPARAM(ref) const FCesiumPropertyArray& array,
    int64 index,
    bool defaultValue) {
  return swl::visit(
      [index, defaultValue](const auto& v) -> bool {
        if (index < 0 || index >= v.size()) {
          return defaultValue;
        }
        auto value = v[index];
        return CesiumGltf::MetadataConversions<bool, decltype(value)>::convert(
                   value)
            .value_or(defaultValue);
      },
      array._value);
}

uint8 UCesiumPropertyArrayBlueprintLibrary::GetByte(
    UPARAM(ref) const FCesiumPropertyArray& array,
    int64 index,
    uint8 defaultValue) {
  return swl::visit(
      [index, defaultValue](const auto& v) -> uint8 {
        if (index < 0 || index >= v.size()) {
          return defaultValue;
        }
        auto value = v[index];
        return CesiumGltf::MetadataConversions<uint8, decltype(value)>::convert(
                   value)
            .value_or(defaultValue);
      },
      array._value);
}

int32 UCesiumPropertyArrayBlueprintLibrary::GetInteger(
    UPARAM(ref) const FCesiumPropertyArray& array,
    int64 index,
    int32 defaultValue) {
  return swl::visit(
      [index, defaultValue](const auto& v) -> int32 {
        if (index < 0 || index >= v.size()) {
          return defaultValue;
        }
        auto value = v[index];
        return CesiumGltf::MetadataConversions<int32, decltype(value)>::convert(
                   value)
            .value_or(defaultValue);
      },
      array._value);
}

int64 UCesiumPropertyArrayBlueprintLibrary::GetInteger64(
    UPARAM(ref) const FCesiumPropertyArray& array,
    int64 index,
    int64 defaultValue) {
  return swl::visit(
      [index, defaultValue](const auto& v) -> int64 {
        if (index < 0 || index >= v.size()) {
          return defaultValue;
        }
        auto value = v[index];
        return CesiumGltf::MetadataConversions<int64_t, decltype(value)>::
            convert(value)
                .value_or(defaultValue);
      },
      array._value);
}

float UCesiumPropertyArrayBlueprintLibrary::GetFloat(
    UPARAM(ref) const FCesiumPropertyArray& array,
    int64 index,
    float defaultValue) {
  return swl::visit(
      [index, defaultValue](const auto& v) -> float {
        if (index < 0 || index >= v.size()) {
          return defaultValue;
        }
        auto value = v[index];
        return CesiumGltf::MetadataConversions<float, decltype(value)>::convert(
                   value)
            .value_or(defaultValue);
      },
      array._value);
}

double UCesiumPropertyArrayBlueprintLibrary::GetFloat64(
    UPARAM(ref) const FCesiumPropertyArray& array,
    int64 index,
    double defaultValue) {
  return swl::visit(
      [index, defaultValue](const auto& v) -> double {
        auto value = v[index];
        return CesiumGltf::MetadataConversions<double, decltype(value)>::
            convert(value)
                .value_or(defaultValue);
      },
      array._value);
}

FString UCesiumPropertyArrayBlueprintLibrary::GetString(
    UPARAM(ref) const FCesiumPropertyArray& array,
    int64 index,
    const FString& defaultValue) {
  return swl::visit(
      [index, defaultValue, &EnumDefinition = array._pEnumDefinition](
          const auto& v) -> FString {
        if (index < 0 || index >= v.size()) {
          return defaultValue;
        }
        auto value = v[index];
        using ValueType = decltype(value);

        if constexpr (CesiumGltf::IsMetadataInteger<ValueType>::value) {
          if (EnumDefinition.IsValid()) {
            TOptional<FString> MaybeName = EnumDefinition->GetName(value);
            if (MaybeName.IsSet()) {
              return MaybeName.GetValue();
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
