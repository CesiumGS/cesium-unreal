// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumPropertyTableProperty.h"
#include "CesiumGltf/PropertyTypeTraits.h"
#include "CesiumMetadataConversions.h"

using namespace CesiumGltf;

ECesiumPropertyTablePropertyStatus
UCesiumPropertyTablePropertyBlueprintLibrary::GetPropertyTablePropertyStatus(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return Property._status;
}

ECesiumMetadataBlueprintType
UCesiumPropertyTablePropertyBlueprintLibrary::GetBlueprintType(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return CesiumMetadataValueTypeToBlueprintType(Property._valueType);
}

ECesiumMetadataBlueprintType
UCesiumPropertyTablePropertyBlueprintLibrary::GetArrayElementBlueprintType(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  if (!Property._valueType.bIsArray) {
    return ECesiumMetadataBlueprintType::None;
  }

  FCesiumMetadataValueType valueType(Property._valueType);
  valueType.bIsArray = false;

  return CesiumMetadataValueTypeToBlueprintType(valueType);
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

int64 UCesiumPropertyTablePropertyBlueprintLibrary::GetArraySize(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return Property._count;
}

bool UCesiumPropertyTablePropertyBlueprintLibrary::GetBoolean(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    bool DefaultValue) {
  return std::visit(
      [FeatureID, DefaultValue](const auto& v) -> bool {
        // size() returns zero if the view is invalid.
        if (FeatureID < 0 || FeatureID >= v.size()) {
          return DefaultValue;
        }
        auto value = v.get(FeatureID);
        return CesiumMetadataConversions<bool, decltype(value)>::convert(
            value,
            DefaultValue);
      },
      Property._property);
}

uint8 UCesiumPropertyTablePropertyBlueprintLibrary::GetByte(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    uint8 DefaultValue) {
  return std::visit(
      [FeatureID, DefaultValue](const auto& v) -> uint8 {
        // size() returns zero if the view is invalid.
        if (FeatureID < 0 || FeatureID >= v.size()) {
          return DefaultValue;
        }
        auto value = v.get(FeatureID);
        return CesiumMetadataConversions<uint8, decltype(value)>::convert(
            value,
            DefaultValue);
      },
      Property._property);
}

int32 UCesiumPropertyTablePropertyBlueprintLibrary::GetInteger(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    int32 DefaultValue) {
  return std::visit(
      [FeatureID, DefaultValue](const auto& v) -> int32 {
        // size() returns zero if the view is invalid.
        if (FeatureID < 0 || FeatureID >= v.size()) {
          return DefaultValue;
        }
        auto value = v.get(FeatureID);
        return CesiumMetadataConversions<int32, decltype(value)>::convert(
            value,
            DefaultValue);
      },
      Property._property);
}

int64 UCesiumPropertyTablePropertyBlueprintLibrary::GetInteger64(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    int64 DefaultValue) {
  return std::visit(
      [FeatureID, DefaultValue](const auto& v) -> int64 {
        // size() returns zero if the view is invalid.
        if (FeatureID < 0 || FeatureID >= v.size()) {
          return DefaultValue;
        }
        auto value = v.get(FeatureID);
        return CesiumMetadataConversions<int64, decltype(value)>::convert(
            value,
            DefaultValue);
      },
      Property._property);
}

float UCesiumPropertyTablePropertyBlueprintLibrary::GetFloat(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    float DefaultValue) {
  return std::visit(
      [FeatureID, DefaultValue](const auto& v) -> float {
        // size() returns zero if the view is invalid.
        if (FeatureID < 0 || FeatureID >= v.size()) {
          return DefaultValue;
        }
        auto value = v.get(FeatureID);
        return CesiumMetadataConversions<float, decltype(value)>::convert(
            value,
            DefaultValue);
      },
      Property._property);
}

double UCesiumPropertyTablePropertyBlueprintLibrary::GetFloat64(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    double DefaultValue) {
  return std::visit(
      [FeatureID, DefaultValue](const auto& v) -> double {
        // size() returns zero if the view is invalid.
        auto value = v.get(FeatureID);
        return CesiumMetadataConversions<double, decltype(value)>::convert(
            value,
            DefaultValue);
      },
      Property._property);
}

FIntPoint UCesiumPropertyTablePropertyBlueprintLibrary::GetIntPoint(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    const FIntPoint& DefaultValue) {
  return std::visit(
      [FeatureID, DefaultValue](const auto& v) -> FIntPoint {
        // size() returns zero if the view is invalid.
        auto value = v.get(FeatureID);
        return CesiumMetadataConversions<FIntPoint, decltype(value)>::convert(
            value,
            DefaultValue);
      },
      Property._property);
}

FVector2D UCesiumPropertyTablePropertyBlueprintLibrary::GetVector2D(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    const FVector2D& DefaultValue) {
  return std::visit(
      [FeatureID, DefaultValue](const auto& v) -> FVector2D {
        // size() returns zero if the view is invalid.
        auto value = v.get(FeatureID);
        return CesiumMetadataConversions<FVector2D, decltype(value)>::convert(
            value,
            DefaultValue);
      },
      Property._property);
}

FString UCesiumPropertyTablePropertyBlueprintLibrary::GetString(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    const FString& DefaultValue) {
  return std::visit(
      [FeatureID, &DefaultValue](const auto& v) -> FString {
        // size() returns zero if the view is invalid.
        if (FeatureID < 0 || FeatureID >= v.size()) {
          return DefaultValue;
        }
        auto value = v.get(FeatureID);
        return CesiumMetadataConversions<FString, decltype(value)>::convert(
            value,
            DefaultValue);
      },
      Property._property);
}

FCesiumPropertyArray UCesiumPropertyTablePropertyBlueprintLibrary::GetArray(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID) {
  return std::visit(
      [FeatureID](const auto& v) -> FCesiumPropertyArray {
        // size() returns zero if the view is invalid.
        if (FeatureID < 0 || FeatureID >= v.size()) {
          return FCesiumPropertyArray();
        }
        auto value = v.get(FeatureID);

        if constexpr (CesiumGltf::IsMetadataArray<decltype(value)>::value) {
          return FCesiumPropertyArray(value);
        }

        return FCesiumPropertyArray();
      },
      Property._property);
}

FCesiumMetadataValue UCesiumPropertyTablePropertyBlueprintLibrary::GetValue(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID) {
  return std::visit(
      [FeatureID](const auto& view) {
        // size() returns zero if the view is invalid.
        if (FeatureID < 0 || FeatureID >= view.size()) {
          return FCesiumMetadataValue();
        }
        return FCesiumMetadataValue(view.get(FeatureID));
      },
      Property._property);
}

bool UCesiumPropertyTablePropertyBlueprintLibrary::IsNormalized(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return Property._normalized;
}
