// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#include "CesiumMetadataValueType.h"

#include <Cesium3DTiles/ClassProperty.h>

using namespace CesiumGltf;

FCesiumMetadataValueType::FCesiumMetadataValueType()
    : Type(ECesiumMetadataType::Invalid),
      ComponentType(ECesiumMetadataComponentType::None),
      bIsArray(false) {}

FCesiumMetadataValueType::FCesiumMetadataValueType(
    ECesiumMetadataType InType,
    ECesiumMetadataComponentType InComponentType,
    bool IsArray)
    : Type(InType), ComponentType(InComponentType), bIsArray(IsArray) {}

FString FCesiumMetadataValueType::ToString() const {
  if (Type == ECesiumMetadataType::Invalid) {
    return TEXT("Invalid Type");
  }

  TArray<FString> strings;
  strings.Reserve(3);

  if (ComponentType != ECesiumMetadataComponentType::None) {
    strings.Emplace(MetadataComponentTypeToString(ComponentType));
  }

  strings.Emplace(MetadataTypeToString(Type));

  if (bIsArray) {
    strings.Emplace("Array");
  }

  return FString::Join(strings, TEXT(" "));
}
