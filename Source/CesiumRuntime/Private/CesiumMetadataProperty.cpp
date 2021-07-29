// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumMetadataProperty.h"
#include "CesiumMetadataConversions.h"

using namespace CesiumGltf;

ECesiumMetadataBlueprintType
UCesiumMetadataPropertyBlueprintLibrary::GetBlueprintType(
    UPARAM(ref) const FCesiumMetadataProperty& Property) {
  return CesiuMetadataTrueTypeToBlueprintType(Property._type);
}

ECesiumMetadataBlueprintType
UCesiumMetadataPropertyBlueprintLibrary::GetBlueprintComponentType(
    UPARAM(ref) const FCesiumMetadataProperty& Property) {
  return CesiuMetadataTrueTypeToBlueprintType(Property._componentType);
}

ECesiumMetadataTrueType UCesiumMetadataPropertyBlueprintLibrary::GetTrueType(
    UPARAM(ref) const FCesiumMetadataProperty& Property) {
  return Property._type;
}

ECesiumMetadataTrueType
UCesiumMetadataPropertyBlueprintLibrary::GetTrueComponentType(
    UPARAM(ref) const FCesiumMetadataProperty& Property) {
  return Property._componentType;
}

int64 UCesiumMetadataPropertyBlueprintLibrary::GetNumberOfFeatures(
    UPARAM(ref) const FCesiumMetadataProperty& Property) {
  return std::visit(
      [](const auto& view) { return view.size(); },
      Property._property);
}

bool UCesiumMetadataPropertyBlueprintLibrary::GetBoolean(
    UPARAM(ref) const FCesiumMetadataProperty& Property,
    int64 featureID,
    bool defaultValue) {
  return std::visit(
      [featureID, defaultValue](const auto& v) -> bool {
        auto value = v.get(featureID);
        return CesiumMetadataConversions<bool, decltype(value)>::convert(
            value,
            defaultValue);
      },
      Property._property);
}

uint8 UCesiumMetadataPropertyBlueprintLibrary::GetByte(
    UPARAM(ref) const FCesiumMetadataProperty& Property,
    int64 featureID,
    uint8 defaultValue) {
  return std::visit(
      [featureID, defaultValue](const auto& v) -> uint8 {
        auto value = v.get(featureID);
        return CesiumMetadataConversions<uint8, decltype(value)>::convert(
            value,
            defaultValue);
      },
      Property._property);
}

int32 UCesiumMetadataPropertyBlueprintLibrary::GetInteger(
    UPARAM(ref) const FCesiumMetadataProperty& Property,
    int64 featureID,
    int32 defaultValue) {
  return std::visit(
      [featureID, defaultValue](const auto& v) -> int32 {
        auto value = v.get(featureID);
        return CesiumMetadataConversions<int32, decltype(value)>::convert(
            value,
            defaultValue);
      },
      Property._property);
}

int64 UCesiumMetadataPropertyBlueprintLibrary::GetInteger64(
    UPARAM(ref) const FCesiumMetadataProperty& Property,
    int64 featureID,
    int64 defaultValue) {
  return std::visit(
      [featureID, defaultValue](const auto& v) -> int64 {
        auto value = v.get(featureID);
        return CesiumMetadataConversions<int64, decltype(value)>::convert(
            value,
            defaultValue);
      },
      Property._property);
}

float UCesiumMetadataPropertyBlueprintLibrary::GetFloat(
    UPARAM(ref) const FCesiumMetadataProperty& Property,
    int64 featureID,
    float defaultValue) {
  return std::visit(
      [featureID, defaultValue](const auto& v) -> float {
        auto value = v.get(featureID);
        return CesiumMetadataConversions<float, decltype(value)>::convert(
            value,
            defaultValue);
      },
      Property._property);
}

FString UCesiumMetadataPropertyBlueprintLibrary::GetString(
    UPARAM(ref) const FCesiumMetadataProperty& Property,
    int64 featureID,
    const FString& defaultValue) {
  return std::visit(
      [featureID, &defaultValue](const auto& v) -> FString {
        auto value = v.get(featureID);
        return CesiumMetadataConversions<FString, decltype(value)>::convert(
            value,
            defaultValue);
      },
      Property._property);
}

FCesiumMetadataArray UCesiumMetadataPropertyBlueprintLibrary::GetArray(
    UPARAM(ref) const FCesiumMetadataProperty& Property,
    int64 featureID) {
  return std::visit(
      [featureID](const auto& v) -> FCesiumMetadataArray {
        auto value = v.get(featureID);
        return CesiumMetadataConversions<
            FCesiumMetadataArray,
            decltype(value)>::convert(value, FCesiumMetadataArray());
      },
      Property._property);
}

FCesiumMetadataGenericValue
UCesiumMetadataPropertyBlueprintLibrary::GetGenericValue(
    UPARAM(ref) const FCesiumMetadataProperty& Property,
    int64 featureID) {
  return std::visit(
      [featureID](const auto& view) {
        return FCesiumMetadataGenericValue{view.get(featureID)};
      },
      Property._property);
}
