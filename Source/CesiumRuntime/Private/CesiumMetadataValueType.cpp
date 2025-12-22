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

namespace {
template <typename TEnum> FString enumToNameString(TEnum value) {
  const UEnum* pEnum = StaticEnum<TEnum>();
  return pEnum ? pEnum->GetNameStringByValue((int64)value) : FString();
}
} // namespace

FString FCesiumMetadataValueType::ToString() const {
  if (Type == ECesiumMetadataType::Invalid) {
    return TEXT("Invalid Type");
  }

  TArray<FString> strings;
  strings.Reserve(3);

  if (ComponentType != ECesiumMetadataComponentType::None) {
    strings.Emplace(enumToNameString(ComponentType));
  }

  strings.Emplace(enumToNameString(Type));

  if (bIsArray) {
    strings.Emplace("Array");
  }

  return FString::Join(strings, TEXT(" "));
}

/*static*/ FCesiumMetadataValueType FCesiumMetadataValueType::fromClassProperty(
    const Cesium3DTiles::ClassProperty& property) {
  CesiumGltf::PropertyType propertyType =
      convertStringToPropertyType(property.type);

  CesiumGltf::PropertyComponentType propertyComponentType =
      property.componentType
          ? convertStringToPropertyComponentType(*property.componentType)
          : CesiumGltf::PropertyComponentType::None;

  return FCesiumMetadataValueType{
      ECesiumMetadataType(propertyType),
      ECesiumMetadataComponentType(propertyComponentType),
      property.array};
}
