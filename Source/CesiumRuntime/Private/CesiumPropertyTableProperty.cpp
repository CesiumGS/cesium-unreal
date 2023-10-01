// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumPropertyTableProperty.h"
#include "CesiumGltf/PropertyTypeTraits.h"
#include "CesiumMetadataConversions.h"

using namespace CesiumGltf;

namespace {
template <typename T, typename Callback>
T callback(
    const std::variant<
        PropertyTablePropertyViewType,
        NormalizedPropertyTablePropertyViewType>& property,
    Callback&& callback) {
  if (std::holds_alternative<CesiumGltf::PropertyTablePropertyViewType>(
          property)) {
    return std::visit(
        callback,
        std::get<CesiumGltf::PropertyTablePropertyViewType>(property));
  }

  return std::visit(
      callback,
      std::get<CesiumGltf::NormalizedPropertyTablePropertyViewType>(property));
}

} // namespace

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
  return callback<int64>(Property._property, [](const auto& view) -> int64 {
    return view.size();
  });
}

int64 UCesiumPropertyTablePropertyBlueprintLibrary::GetArraySize(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return callback<int64>(Property._property, [](const auto& view) -> int64 {
    return view.arrayCount();
  });
}

bool UCesiumPropertyTablePropertyBlueprintLibrary::GetBoolean(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    bool DefaultValue) {
  return callback<bool>(
      Property._property,
      [FeatureID, DefaultValue](const auto& v) -> bool {
        // size() returns zero if the view is invalid.
        if (FeatureID < 0 || FeatureID >= v.size()) {
          return DefaultValue;
        }
        auto maybeValue = v.get(FeatureID);
        if (maybeValue) {
          auto value = *maybeValue;
          return CesiumMetadataConversions<bool, decltype(value)>::convert(
              value,
              DefaultValue);
        }
        return DefaultValue;
      });
}

uint8 UCesiumPropertyTablePropertyBlueprintLibrary::GetByte(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    uint8 DefaultValue) {
  return callback<uint8>(
      Property._property,
      [FeatureID, DefaultValue](const auto& v) -> uint8 {
        // size() returns zero if the view is invalid.
        if (FeatureID < 0 || FeatureID >= v.size()) {
          return DefaultValue;
        }
        auto maybeValue = v.get(FeatureID);
        if (maybeValue) {
          auto value = *maybeValue;
          return CesiumMetadataConversions<uint8, decltype(value)>::convert(
              value,
              DefaultValue);
        }
        return DefaultValue;
      });
}

int32 UCesiumPropertyTablePropertyBlueprintLibrary::GetInteger(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    int32 DefaultValue) {
  return callback<int32>(
      Property._property,
      [FeatureID, DefaultValue](const auto& v) -> int32 {
        // size() returns zero if the view is invalid.
        if (FeatureID < 0 || FeatureID >= v.size()) {
          return DefaultValue;
        }
        auto maybeValue = v.get(FeatureID);
        if (maybeValue) {
          auto value = *maybeValue;
          return CesiumMetadataConversions<int32, decltype(value)>::convert(
              value,
              DefaultValue);
        }
        return DefaultValue;
      });
}

int64 UCesiumPropertyTablePropertyBlueprintLibrary::GetInteger64(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    int64 DefaultValue) {
  return callback<int64>(
      Property._property,
      [FeatureID, DefaultValue](const auto& v) -> int64 {
        // size() returns zero if the view is invalid.
        if (FeatureID < 0 || FeatureID >= v.size()) {
          return DefaultValue;
        }
        auto maybeValue = v.get(FeatureID);
        if (maybeValue) {
          auto value = *maybeValue;
          return CesiumMetadataConversions<int64, decltype(value)>::convert(
              value,
              DefaultValue);
        }
        return DefaultValue;
      });
}

float UCesiumPropertyTablePropertyBlueprintLibrary::GetFloat(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    float DefaultValue) {
  return callback<float>(
      Property._property,
      [FeatureID, DefaultValue](const auto& v) -> float {
        // size() returns zero if the view is invalid.
        if (FeatureID < 0 || FeatureID >= v.size()) {
          return DefaultValue;
        }
        auto maybeValue = v.get(FeatureID);
        if (maybeValue) {
          auto value = *maybeValue;
          return CesiumMetadataConversions<float, decltype(value)>::convert(
              value,
              DefaultValue);
        }
        return DefaultValue;
      });
}

double UCesiumPropertyTablePropertyBlueprintLibrary::GetFloat64(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    double DefaultValue) {
  return callback<double>(
      Property._property,
      [FeatureID, DefaultValue](const auto& v) -> double {
        // size() returns zero if the view is invalid.
        if (FeatureID < 0 || FeatureID >= v.size()) {
          return DefaultValue;
        }
        auto maybeValue = v.get(FeatureID);
        if (maybeValue) {
          auto value = *maybeValue;
          return CesiumMetadataConversions<double, decltype(value)>::convert(
              value,
              DefaultValue);
        }
        return DefaultValue;
      });
}

FIntPoint UCesiumPropertyTablePropertyBlueprintLibrary::GetIntPoint(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    const FIntPoint& DefaultValue) {
  return callback<FIntPoint>(
      Property._property,
      [FeatureID, DefaultValue](const auto& v) -> FIntPoint {
        // size() returns zero if the view is invalid.
        if (FeatureID < 0 || FeatureID >= v.size()) {
          return DefaultValue;
        }
        auto maybeValue = v.get(FeatureID);
        if (maybeValue) {
          auto value = *maybeValue;
          return CesiumMetadataConversions<FIntPoint, decltype(value)>::convert(
              value,
              DefaultValue);
        }
        return DefaultValue;
      });
}

FVector2D UCesiumPropertyTablePropertyBlueprintLibrary::GetVector2D(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    const FVector2D& DefaultValue) {
  return callback<FVector2D>(
      Property._property,
      [FeatureID, DefaultValue](const auto& v) -> FVector2D {
        // size() returns zero if the view is invalid.
        if (FeatureID < 0 || FeatureID >= v.size()) {
          return DefaultValue;
        }
        auto maybeValue = v.get(FeatureID);
        if (maybeValue) {
          auto value = *maybeValue;
          return CesiumMetadataConversions<FVector2D, decltype(value)>::convert(
              value,
              DefaultValue);
        }
        return DefaultValue;
      });
}

FIntVector UCesiumPropertyTablePropertyBlueprintLibrary::GetIntVector(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    const FIntVector& DefaultValue) {
  return callback<FIntVector>(
      Property._property,
      [FeatureID, DefaultValue](const auto& v) -> FIntVector {
        // size() returns zero if the view is invalid.
        if (FeatureID < 0 || FeatureID >= v.size()) {
          return DefaultValue;
        }
        auto maybeValue = v.get(FeatureID);
        if (maybeValue) {
          auto value = *maybeValue;
          return CesiumMetadataConversions<FIntVector, decltype(value)>::
              convert(value, DefaultValue);
        }
        return DefaultValue;
      });
}

FVector3f UCesiumPropertyTablePropertyBlueprintLibrary::GetVector3f(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    const FVector3f& DefaultValue) {
  return callback<FVector3f>(
      Property._property,
      [FeatureID, DefaultValue](const auto& v) -> FVector3f {
        // size() returns zero if the view is invalid.
        if (FeatureID < 0 || FeatureID >= v.size()) {
          return DefaultValue;
        }
        auto maybeValue = v.get(FeatureID);
        if (maybeValue) {
          auto value = *maybeValue;
          return CesiumMetadataConversions<FVector3f, decltype(value)>::convert(
              value,
              DefaultValue);
        }
        return DefaultValue;
      });
}

FVector UCesiumPropertyTablePropertyBlueprintLibrary::GetVector(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    const FVector& DefaultValue) {
  return callback<FVector>(
      Property._property,
      [FeatureID, DefaultValue](const auto& v) -> FVector {
        // size() returns zero if the view is invalid.
        if (FeatureID < 0 || FeatureID >= v.size()) {
          return DefaultValue;
        }
        auto maybeValue = v.get(FeatureID);
        if (maybeValue) {
          auto value = *maybeValue;
          return CesiumMetadataConversions<FVector, decltype(value)>::convert(
              value,
              DefaultValue);
        }
        return DefaultValue;
      });
}

FVector4 UCesiumPropertyTablePropertyBlueprintLibrary::GetVector4(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    const FVector4& DefaultValue) {
  return callback<FVector4>(
      Property._property,
      [FeatureID, DefaultValue](const auto& v) -> FVector4 {
        // size() returns zero if the view is invalid.
        if (FeatureID < 0 || FeatureID >= v.size()) {
          return DefaultValue;
        }
        auto maybeValue = v.get(FeatureID);
        if (maybeValue) {
          auto value = *maybeValue;
          return CesiumMetadataConversions<FVector4, decltype(value)>::convert(
              value,
              DefaultValue);
        }
        return DefaultValue;
      });
}

FMatrix UCesiumPropertyTablePropertyBlueprintLibrary::GetMatrix(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    const FMatrix& DefaultValue) {
  return callback<FMatrix>(
      Property._property,
      [FeatureID, DefaultValue](const auto& v) -> FMatrix {
        // size() returns zero if the view is invalid.
        if (FeatureID < 0 || FeatureID >= v.size()) {
          return DefaultValue;
        }
        auto maybeValue = v.get(FeatureID);
        if (maybeValue) {
          auto value = *maybeValue;
          return CesiumMetadataConversions<FMatrix, decltype(value)>::convert(
              value,
              DefaultValue);
        }
        return DefaultValue;
      });
}

FString UCesiumPropertyTablePropertyBlueprintLibrary::GetString(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID,
    const FString& DefaultValue) {
  return callback<FString>(
      Property._property,
      [FeatureID, &DefaultValue](const auto& v) -> FString {
        // size() returns zero if the view is invalid.
        if (FeatureID < 0 || FeatureID >= v.size()) {
          return DefaultValue;
        }
        auto maybeValue = v.get(FeatureID);
        if (maybeValue) {
          auto value = *maybeValue;
          return CesiumMetadataConversions<FString, decltype(value)>::convert(
              value,
              DefaultValue);
        }
        return DefaultValue;
      });
}

FCesiumPropertyArray UCesiumPropertyTablePropertyBlueprintLibrary::GetArray(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID) {
  return callback<FCesiumPropertyArray>(
      Property._property,
      [FeatureID](const auto& v) -> FCesiumPropertyArray {
        // size() returns zero if the view is invalid.
        if (FeatureID < 0 || FeatureID >= v.size()) {
          return FCesiumPropertyArray();
        }
        auto maybeValue = v.get(FeatureID);
        if (maybeValue) {
          auto value = *maybeValue;
          if constexpr (CesiumGltf::IsMetadataArray<decltype(value)>::value) {
            return FCesiumPropertyArray(value);
          }
        }
        return FCesiumPropertyArray();
      });
}

FCesiumMetadataValue UCesiumPropertyTablePropertyBlueprintLibrary::GetValue(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID) {
  return callback<FCesiumMetadataValue>(
      Property._property,
      [FeatureID](const auto& view) -> FCesiumMetadataValue {
        // size() returns zero if the view is invalid.
        if (FeatureID >= 0 && FeatureID < view.size()) {
          return FCesiumMetadataValue(view.get(FeatureID));
        }
        return FCesiumMetadataValue();
      });
}

FCesiumMetadataValue UCesiumPropertyTablePropertyBlueprintLibrary::GetRawValue(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID) {
  return callback<FCesiumMetadataValue>(
      Property._property,
      [FeatureID](const auto& view) -> FCesiumMetadataValue {
        // Return an empty value if the property is empty.
        if (view.status() ==
            PropertyTablePropertyViewStatus::EmptyPropertyWithDefault) {
          return FCesiumMetadataValue();
        }

        // size() returns zero if the view is invalid.
        if (FeatureID >= 0 && FeatureID < view.size()) {
          return FCesiumMetadataValue(view.getRaw(FeatureID));
        }

        return FCesiumMetadataValue();
      });
}

bool UCesiumPropertyTablePropertyBlueprintLibrary::IsNormalized(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return Property._normalized;
}

FCesiumMetadataValue UCesiumPropertyTablePropertyBlueprintLibrary::GetOffset(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return callback<FCesiumMetadataValue>(
      Property._property,
      [](const auto& view) -> FCesiumMetadataValue {
        // Returns an empty value if no offset is specified.
        return FCesiumMetadataValue(view.offset());
      });
}

FCesiumMetadataValue UCesiumPropertyTablePropertyBlueprintLibrary::GetScale(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return callback<FCesiumMetadataValue>(
      Property._property,
      [](const auto& view) -> FCesiumMetadataValue {
        // Returns an empty value if no scale is specified.
        return FCesiumMetadataValue(view.scale());
      });
}

FCesiumMetadataValue
UCesiumPropertyTablePropertyBlueprintLibrary::GetMinimumValue(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return callback<FCesiumMetadataValue>(
      Property._property,
      [](const auto& view) -> FCesiumMetadataValue {
        // Returns an empty value if no min is specified.
        return FCesiumMetadataValue(view.min());
      });
}

FCesiumMetadataValue
UCesiumPropertyTablePropertyBlueprintLibrary::GetMaximumValue(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return callback<FCesiumMetadataValue>(
      Property._property,
      [](const auto& view) -> FCesiumMetadataValue {
        // Returns an empty value if no max is specified.
        return FCesiumMetadataValue(view.max());
      });
}

FCesiumMetadataValue
UCesiumPropertyTablePropertyBlueprintLibrary::GetNoDataValue(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return callback<FCesiumMetadataValue>(
      Property._property,
      [](const auto& view) -> FCesiumMetadataValue {
        // Returns an empty value if no "no data" value is specified.
        return FCesiumMetadataValue(view.noData());
      });
}

FCesiumMetadataValue
UCesiumPropertyTablePropertyBlueprintLibrary::GetDefaultValue(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return callback<FCesiumMetadataValue>(
      Property._property,
      [](const auto& view) -> FCesiumMetadataValue {
        // Returns an empty value if no default value is specified.
        return FCesiumMetadataValue(view.defaultValue());
      });
}

PRAGMA_DISABLE_DEPRECATION_WARNINGS

ECesiumMetadataBlueprintType
UCesiumPropertyTablePropertyBlueprintLibrary::GetBlueprintComponentType(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return UCesiumPropertyTablePropertyBlueprintLibrary::
      GetArrayElementBlueprintType(Property);
}

ECesiumMetadataTrueType_DEPRECATED
UCesiumPropertyTablePropertyBlueprintLibrary::GetTrueType(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return CesiumMetadataValueTypeToTrueType(Property._valueType);
}

ECesiumMetadataTrueType_DEPRECATED
UCesiumPropertyTablePropertyBlueprintLibrary::GetTrueComponentType(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  FCesiumMetadataValueType type = Property._valueType;
  type.bIsArray = false;
  return CesiumMetadataValueTypeToTrueType(type);
}

int64 UCesiumPropertyTablePropertyBlueprintLibrary::GetNumberOfFeatures(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return UCesiumPropertyTablePropertyBlueprintLibrary::GetPropertySize(
      Property);
}

int64 UCesiumPropertyTablePropertyBlueprintLibrary::GetComponentCount(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property) {
  return UCesiumPropertyTablePropertyBlueprintLibrary::GetArraySize(Property);
}

FCesiumMetadataValue
UCesiumPropertyTablePropertyBlueprintLibrary::GetGenericValue(
    UPARAM(ref) const FCesiumPropertyTableProperty& Property,
    int64 FeatureID) {
  return UCesiumPropertyTablePropertyBlueprintLibrary::GetValue(
      Property,
      FeatureID);
}

PRAGMA_ENABLE_DEPRECATION_WARNINGS
