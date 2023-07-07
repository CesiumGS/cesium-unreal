// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumPropertyTableProperty.h"
#include "CesiumGltf/PropertyTypeTraits.h"
#include "CesiumMetadataConversions.h"

using namespace CesiumGltf;

ECesiumMetadataBlueprintType
UCesiumPropertyTablePropertyBlueprintLibrary::GetBlueprintType(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return CesiumMetadataTypesToBlueprintType(Property._valueType);
}

ECesiumMetadataBlueprintType
UCesiumPropertyTablePropertyBlueprintLibrary::GetArrayElementBlueprintType(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  if (!Property._valueType.bIsArray) {
    return ECesiumMetadataBlueprintType::None;
  }

  FCesiumMetadataValueType valueType(Property._valueType);
  valueType.bIsArray = false;

  return CesiumMetadataTypesToBlueprintType(valueType);
}

FCesiumMetadataValueType
UCesiumPropertyTablePropertyBlueprintLibrary::GetValueType(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return Property._valueType;
}

int64 UCesiumPropertyTablePropertyBlueprintLibrary::GetPropertySize(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return std::visit(
      [](const auto& view) { return view.size(); },
      Property._property);
}

int64
UCesiumPropertyTablePropertyBlueprintLibrary::GetPropertyArraySize(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return Property._count;
}

bool UCesiumPropertyTablePropertyBlueprintLibrary::GetBoolean(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 featureID,
    bool defaultValue) {
  return std::visit(
      [featureID, defaultValue](const auto& v) -> bool {
        if (featureID < 0 || featureID >= v.size()) {
          return defaultValue;
        }
        auto value = v.get(featureID);
        return CesiumMetadataConversions<bool, decltype(value)>::convert(
            value,
            defaultValue);
      },
      Property._property);
}

uint8 UCesiumPropertyTablePropertyBlueprintLibrary::GetByte(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 featureID,
    uint8 defaultValue) {
  return std::visit(
      [featureID, defaultValue](const auto& v) -> uint8 {
        if (featureID < 0 || featureID >= v.size()) {
          return defaultValue;
        }
        auto value = v.get(featureID);
        return CesiumMetadataConversions<uint8, decltype(value)>::convert(
            value,
            defaultValue);
      },
      Property._property);
}

int32 UCesiumPropertyTablePropertyBlueprintLibrary::GetInteger(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 featureID,
    int32 defaultValue) {
  return std::visit(
      [featureID, defaultValue](const auto& v) -> int32 {
        if (featureID < 0 || featureID >= v.size()) {
          return defaultValue;
        }
        auto value = v.get(featureID);
        return CesiumMetadataConversions<int32, decltype(value)>::convert(
            value,
            defaultValue);
      },
      Property._property);
}

int64 UCesiumPropertyTablePropertyBlueprintLibrary::GetInteger64(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 featureID,
    int64 defaultValue) {
  return std::visit(
      [featureID, defaultValue](const auto& v) -> int64 {
        if (featureID < 0 || featureID >= v.size()) {
          return defaultValue;
        }
        auto value = v.get(featureID);
        return CesiumMetadataConversions<int64, decltype(value)>::convert(
            value,
            defaultValue);
      },
      Property._property);
}

float UCesiumPropertyTablePropertyBlueprintLibrary::GetFloat(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 featureID,
    float defaultValue) {
  return std::visit(
      [featureID, defaultValue](const auto& v) -> float {
        if (featureID < 0 || featureID >= v.size()) {
          return defaultValue;
        }
        auto value = v.get(featureID);
        return CesiumMetadataConversions<float, decltype(value)>::convert(
            value,
            defaultValue);
      },
      Property._property);
}

double UCesiumPropertyTablePropertyBlueprintLibrary::GetFloat64(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 featureID,
    double defaultValue) {
  return std::visit(
      [featureID, defaultValue](const auto& v) -> double {
        auto value = v.get(featureID);
        return CesiumMetadataConversions<double, decltype(value)>::convert(
            value,
            defaultValue);
      },
      Property._property);
}

FString UCesiumPropertyTablePropertyBlueprintLibrary::GetString(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 featureID,
    const FString& defaultValue) {
  return std::visit(
      [featureID, &defaultValue](const auto& v) -> FString {
        if (featureID < 0 || featureID >= v.size()) {
          return defaultValue;
        }
        auto value = v.get(featureID);
        return CesiumMetadataConversions<FString, decltype(value)>::convert(
            value,
            defaultValue);
      },
      Property._property);
}

FCesiumMetadataArray UCesiumPropertyTablePropertyBlueprintLibrary::GetArray(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 featureID) {
  return std::visit(
      [featureID](const auto& v) -> FCesiumMetadataArray {
        if (featureID < 0 || featureID >= v.size()) {
          return FCesiumMetadataArray();
        }
        auto value = v.get(featureID);

        auto createArrayView = [](const auto& array) -> FCesiumMetadataArray {
          return FCesiumMetadataArray(array);
        };

        // TODO: Is this the best way?
        if constexpr (CesiumGltf::IsMetadataArray<decltype(value)>::value) {
          return createArrayView(value);
        }

        return FCesiumMetadataArray();
      },
      Property._property);
}

FCesiumMetadataValue
UCesiumPropertyTablePropertyBlueprintLibrary::GetValue(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 featureID) {
  return std::visit(
      [featureID](const auto& view) {
        if (featureID < 0 || featureID >= view.size()) {
          return FCesiumMetadataValue();
        }
        return FCesiumMetadataValue(view.get(featureID));
      },
      Property._property);
}

bool UCesiumPropertyTablePropertyBlueprintLibrary::IsNormalized(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return Property._normalized;
}
