// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumMetadataValueType.h"

FCesiumMetadataValueType::FCesiumMetadataValueType() noexcept
    : Type(ECesiumMetadataType::Invalid),
      ComponentType(ECesiumMetadataComponentType::None),
      bIsArray(false) {}

FCesiumMetadataValueType::FCesiumMetadataValueType(
    ECesiumMetadataType InType,
    ECesiumMetadataComponentType InComponentType,
    bool IsArray) noexcept
    : Type(InType), ComponentType(InComponentType), bIsArray(IsArray) {}

bool FCesiumMetadataValueType::operator==(
    const FCesiumMetadataValueType& ValueType) const noexcept {
  return Type == ValueType.Type && ComponentType == ValueType.ComponentType &&
         bIsArray == ValueType.bIsArray;
}

bool FCesiumMetadataValueType::operator!=(
    const FCesiumMetadataValueType& ValueType) const noexcept {
  return Type != ValueType.Type || ComponentType != ValueType.ComponentType ||
         bIsArray != ValueType.bIsArray;
}
