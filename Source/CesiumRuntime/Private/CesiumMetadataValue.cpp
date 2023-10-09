// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumMetadataValue.h"
#include "CesiumGltf/PropertyTypeTraits.h"
#include "CesiumMetadataConversions.h"
#include "CesiumPropertyArray.h"

ECesiumMetadataBlueprintType
UCesiumMetadataValueBlueprintLibrary::GetBlueprintType(
    UPARAM(ref) const FCesiumMetadataValue& Value) {
  return CesiumMetadataValueTypeToBlueprintType(Value._valueType);
}

ECesiumMetadataBlueprintType
UCesiumMetadataValueBlueprintLibrary::GetArrayElementBlueprintType(
    UPARAM(ref) const FCesiumMetadataValue& Value) {
  if (!Value._valueType.bIsArray) {
    return ECesiumMetadataBlueprintType::None;
  }

  FCesiumMetadataValueType types(Value._valueType);
  types.bIsArray = false;

  return CesiumMetadataValueTypeToBlueprintType(types);
}

FCesiumMetadataValueType UCesiumMetadataValueBlueprintLibrary::GetValueType(
    UPARAM(ref) const FCesiumMetadataValue& Value) {
  return Value._valueType;
}

PRAGMA_DISABLE_DEPRECATION_WARNINGS

ECesiumMetadataTrueType_DEPRECATED
UCesiumMetadataValueBlueprintLibrary::GetTrueType(
    UPARAM(ref) const FCesiumMetadataValue& Value) {
  return CesiumMetadataValueTypeToTrueType(Value._valueType);
}

ECesiumMetadataTrueType_DEPRECATED
UCesiumMetadataValueBlueprintLibrary::GetTrueComponentType(
    UPARAM(ref) const FCesiumMetadataValue& Value) {
  FCesiumMetadataValueType type = Value._valueType;
  type.bIsArray = false;
  return CesiumMetadataValueTypeToTrueType(type);
}

bool UCesiumMetadataValueBlueprintLibrary::GetBoolean(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    bool DefaultValue) {
  return FCesiumMetadataValue::convertTo(Value._value, DefaultValue);
}

PRAGMA_ENABLE_DEPRECATION_WARNINGS

uint8 UCesiumMetadataValueBlueprintLibrary::GetByte(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    uint8 DefaultValue) {
  return FCesiumMetadataValue::convertTo(Value._value, DefaultValue);
}

int32 UCesiumMetadataValueBlueprintLibrary::GetInteger(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    int32 DefaultValue) {
  return FCesiumMetadataValue::convertTo(Value._value, DefaultValue);
}

int64 UCesiumMetadataValueBlueprintLibrary::GetInteger64(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    int64 DefaultValue) {
  return FCesiumMetadataValue::convertTo(Value._value, DefaultValue);
}

float UCesiumMetadataValueBlueprintLibrary::GetFloat(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    float DefaultValue) {
  return FCesiumMetadataValue::convertTo(Value._value, DefaultValue);
}

double UCesiumMetadataValueBlueprintLibrary::GetFloat64(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    double DefaultValue) {
  return FCesiumMetadataValue::convertTo(Value._value, DefaultValue);
}

FIntPoint UCesiumMetadataValueBlueprintLibrary::GetIntPoint(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    const FIntPoint& DefaultValue) {
  return FCesiumMetadataValue::convertTo(Value._value, DefaultValue);
}

FVector2D UCesiumMetadataValueBlueprintLibrary::GetVector2D(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    const FVector2D& DefaultValue) {
  return FCesiumMetadataValue::convertTo(Value._value, DefaultValue);
}

FIntVector UCesiumMetadataValueBlueprintLibrary::GetIntVector(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    const FIntVector& DefaultValue) {
  return FCesiumMetadataValue::convertTo(Value._value, DefaultValue);
}

FVector3f UCesiumMetadataValueBlueprintLibrary::GetVector3f(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    const FVector3f& DefaultValue) {
  return FCesiumMetadataValue::convertTo(Value._value, DefaultValue);
}

FVector UCesiumMetadataValueBlueprintLibrary::GetVector(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    const FVector& DefaultValue) {
  return FCesiumMetadataValue::convertTo(Value._value, DefaultValue);
}

FVector4 UCesiumMetadataValueBlueprintLibrary::GetVector4(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    const FVector4& DefaultValue) {
  return FCesiumMetadataValue::convertTo(Value._value, DefaultValue);
}

FMatrix UCesiumMetadataValueBlueprintLibrary::GetMatrix(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    const FMatrix& DefaultValue) {
  return FCesiumMetadataValue::convertTo(Value._value, DefaultValue);
}

FString UCesiumMetadataValueBlueprintLibrary::GetString(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    const FString& DefaultValue) {
  return FCesiumMetadataValue::convertTo(Value._value, DefaultValue);
}

FCesiumPropertyArray UCesiumMetadataValueBlueprintLibrary::GetArray(
    UPARAM(ref) const FCesiumMetadataValue& Value) {
  return mpark::visit(
      [](auto value) -> FCesiumPropertyArray {
        if constexpr (CesiumGltf::IsMetadataArray<decltype(value)>::value) {
          return FCesiumPropertyArray(value);
        }
        return FCesiumPropertyArray();
      },
      Value._value);
}

bool UCesiumMetadataValueBlueprintLibrary::IsEmpty(
    UPARAM(ref) const FCesiumMetadataValue& Value) {
  return mpark::holds_alternative<mpark::monostate>(Value._value);
}

template <typename TTo>
/*static*/ TTo FCesiumMetadataValue::convertTo(
    const ValueType& Value,
    const TTo& DefaultValue) noexcept {
  return mpark::visit(
      [DefaultValue](auto value) {
        return CesiumMetadataConversions<TTo, decltype(value)>::convert(
            value,
            DefaultValue);
      },
      Value);
}

FCesiumMetadataValue& FCesiumMetadataValue::operator=(
    const FCesiumMetadataValue& rhs) noexcept = default;
FCesiumMetadataValue&
FCesiumMetadataValue::operator=(FCesiumMetadataValue&& rhs) noexcept = default;
FCesiumMetadataValue::FCesiumMetadataValue(
    const FCesiumMetadataValue& rhs) noexcept = default;
FCesiumMetadataValue::FCesiumMetadataValue(
    FCesiumMetadataValue&& rhs) noexcept = default;
FCesiumMetadataValue ::~FCesiumMetadataValue() noexcept = default;
