// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumPropertyArrayBlueprintLibrary.h"
#include "UnrealMetadataConversions.h"
#include <CesiumGltf/MetadataConversions.h>

ECesiumMetadataBlueprintType
UCesiumPropertyArrayBlueprintLibrary::GetElementBlueprintType(
    UPARAM(ref) const FCesiumPropertyArray& Array) {
  return CesiumMetadataValueTypeToBlueprintType(Array._elementType);
}

ECesiumMetadataBlueprintType
UCesiumPropertyArrayBlueprintLibrary::GetBlueprintComponentType(
    UPARAM(ref) const FCesiumPropertyArray& Array) {
  return CesiumMetadataValueTypeToBlueprintType(Array._elementType);
}

FCesiumMetadataValueType
UCesiumPropertyArrayBlueprintLibrary::GetElementValueType(
    UPARAM(ref) const FCesiumPropertyArray& Array) {
  return Array._elementType;
}

int64 UCesiumPropertyArrayBlueprintLibrary::GetArraySize(
    UPARAM(ref) const FCesiumPropertyArray& Array) {
  if (Array._valueArray.IsSet()) {
    return Array._valueArray->Num();
  }

  return swl::visit(
      [](const auto& view) { return view.size(); },
      Array._valueView);
}

int64 UCesiumPropertyArrayBlueprintLibrary::GetSize(
    UPARAM(ref) const FCesiumPropertyArray& Array) {
  return UCesiumPropertyArrayBlueprintLibrary::GetArraySize(Array);
}

FCesiumMetadataValue UCesiumPropertyArrayBlueprintLibrary::GetValue(
    UPARAM(ref) const FCesiumPropertyArray& Array,
    int64 Index) {
  int64 arraySize = UCesiumPropertyArrayBlueprintLibrary::GetArraySize(Array);
  if (Index < 0 || Index >= arraySize) {
    FFrame::KismetExecutionMessage(
        *FString::Printf(
            TEXT(
                "Attempted to access index %d from CesiumPropertyArray of length %d!"),
            Index,
            arraySize),
        ELogVerbosity::Warning,
        FName("CesiumPropertyArrayOutOfBoundsWarning"));
    return FCesiumMetadataValue();
  }

  if (Array._valueArray.IsSet()) {
    return (*Array._valueArray)[Index];
  }

  return swl::visit(
      [Index, &pEnumDefinition = Array._pEnumDefinition](
          const auto& v) -> FCesiumMetadataValue {
        return FCesiumMetadataValue(v[Index], pEnumDefinition);
      },
      Array._valueView);
}

PRAGMA_DISABLE_DEPRECATION_WARNINGS
ECesiumMetadataTrueType_DEPRECATED
UCesiumPropertyArrayBlueprintLibrary::GetTrueComponentType(
    UPARAM(ref) const FCesiumPropertyArray& array) {
  return CesiumMetadataValueTypeToTrueType(array._elementType);
}

bool UCesiumPropertyArrayBlueprintLibrary::GetBoolean(
    UPARAM(ref) const FCesiumPropertyArray& array,
    int64 index,
    bool defaultValue) {
  FCesiumMetadataValue value =
      UCesiumPropertyArrayBlueprintLibrary::GetValue(array, index);
  return UCesiumMetadataValueBlueprintLibrary::GetBoolean(value, defaultValue);
}

uint8 UCesiumPropertyArrayBlueprintLibrary::GetByte(
    UPARAM(ref) const FCesiumPropertyArray& array,
    int64 index,
    uint8 defaultValue) {
  FCesiumMetadataValue value =
      UCesiumPropertyArrayBlueprintLibrary::GetValue(array, index);
  return UCesiumMetadataValueBlueprintLibrary::GetByte(value, defaultValue);
}

int32 UCesiumPropertyArrayBlueprintLibrary::GetInteger(
    UPARAM(ref) const FCesiumPropertyArray& array,
    int64 index,
    int32 defaultValue) {
  FCesiumMetadataValue value =
      UCesiumPropertyArrayBlueprintLibrary::GetValue(array, index);
  return UCesiumMetadataValueBlueprintLibrary::GetInteger(value, defaultValue);
}

int64 UCesiumPropertyArrayBlueprintLibrary::GetInteger64(
    UPARAM(ref) const FCesiumPropertyArray& array,
    int64 index,
    int64 defaultValue) {
  FCesiumMetadataValue value =
      UCesiumPropertyArrayBlueprintLibrary::GetValue(array, index);
  return UCesiumMetadataValueBlueprintLibrary::GetInteger64(
      value,
      defaultValue);
}

float UCesiumPropertyArrayBlueprintLibrary::GetFloat(
    UPARAM(ref) const FCesiumPropertyArray& array,
    int64 index,
    float defaultValue) {
  FCesiumMetadataValue value =
      UCesiumPropertyArrayBlueprintLibrary::GetValue(array, index);
  return UCesiumMetadataValueBlueprintLibrary::GetFloat(value, defaultValue);
}

double UCesiumPropertyArrayBlueprintLibrary::GetFloat64(
    UPARAM(ref) const FCesiumPropertyArray& array,
    int64 index,
    double defaultValue) {
  FCesiumMetadataValue value =
      UCesiumPropertyArrayBlueprintLibrary::GetValue(array, index);
  return UCesiumMetadataValueBlueprintLibrary::GetFloat64(value, defaultValue);
}

FString UCesiumPropertyArrayBlueprintLibrary::GetString(
    UPARAM(ref) const FCesiumPropertyArray& array,
    int64 index,
    const FString& defaultValue) {
  FCesiumMetadataValue value =
      UCesiumPropertyArrayBlueprintLibrary::GetValue(array, index);
  return UCesiumMetadataValueBlueprintLibrary::GetString(value, defaultValue);
}
PRAGMA_ENABLE_DEPRECATION_WARNINGS
