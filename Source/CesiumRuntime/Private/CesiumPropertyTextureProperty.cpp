// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumPropertyTextureProperty.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumMetadataConversions.h"

#include <cstdint>
#include <limits>

using namespace CesiumGltf;

namespace {
template <typename T, typename Callback>
T callback(
    const std::variant<
        PropertyTexturePropertyViewType,
        NormalizedPropertyTexturePropertyViewType>& property,
    Callback&& callback) {
  if (std::holds_alternative<CesiumGltf::PropertyTexturePropertyViewType>(
          property)) {
    return std::visit(
        callback,
        std::get<CesiumGltf::PropertyTexturePropertyViewType>(property));
  }

  return std::visit(
      callback,
      std::get<CesiumGltf::NormalizedPropertyTexturePropertyViewType>(
          property));
}

} // namespace

const int64 FCesiumPropertyTextureProperty::getTexCoordSetIndex() const {
  return callback<int64>(this->_property, [](const auto& view) -> int64 {
    return view.getTexCoordSetIndex();
  });
}

const CesiumGltf::Sampler* FCesiumPropertyTextureProperty::getSampler() const {
  return callback<const CesiumGltf::Sampler*>(
      this->_property,
      [](const auto& view) -> const CesiumGltf::Sampler* {
        return view.getSampler();
      });
}

const CesiumGltf::ImageCesium*
FCesiumPropertyTextureProperty::getImage() const {
  return callback<const CesiumGltf::ImageCesium*>(
      this->_property,
      [](const auto& view) -> const CesiumGltf::ImageCesium* {
        return view.getImage();
      });
}

ECesiumPropertyTexturePropertyStatus
UCesiumPropertyTexturePropertyBlueprintLibrary::
    GetPropertyTexturePropertyStatus(
        UPARAM(ref) const FCesiumPropertyTextureProperty& Property) {
  return Property._status;
}

ECesiumMetadataBlueprintType
UCesiumPropertyTexturePropertyBlueprintLibrary::GetBlueprintType(
    UPARAM(ref) const FCesiumPropertyTextureProperty& Property) {
  return CesiumMetadataValueTypeToBlueprintType(Property._valueType);
}

ECesiumMetadataBlueprintType
UCesiumPropertyTexturePropertyBlueprintLibrary::GetArrayElementBlueprintType(
    UPARAM(ref) const FCesiumPropertyTextureProperty& Property) {
  if (!Property._valueType.bIsArray) {
    return ECesiumMetadataBlueprintType::None;
  }

  FCesiumMetadataValueType valueType(Property._valueType);
  valueType.bIsArray = false;

  return CesiumMetadataValueTypeToBlueprintType(valueType);
}

FCesiumMetadataValueType
UCesiumPropertyTexturePropertyBlueprintLibrary::GetValueType(
    UPARAM(ref) const FCesiumPropertyTextureProperty& Property) {
  return Property._valueType;
}

int64 UCesiumPropertyTexturePropertyBlueprintLibrary::GetArraySize(
    UPARAM(ref) const FCesiumPropertyTextureProperty& Property) {
  return callback<int64>(Property._property, [](const auto& view) -> int64 {
    return view.arrayCount();
  });
}

int64 UCesiumPropertyTexturePropertyBlueprintLibrary::GetTextureCoordinateIndex(
    const UPrimitiveComponent* Component,
    UPARAM(ref) const FCesiumPropertyTextureProperty& Property) {
  const UCesiumGltfPrimitiveComponent* pPrimitive =
      Cast<UCesiumGltfPrimitiveComponent>(Component);
  if (!pPrimitive) {
    return -1;
  }

  int64_t texCoordSetIndex =
      callback<int64_t>(Property._property, [](const auto& view) {
        return view.getTexCoordSetIndex();
      });

  auto textureCoordinateIndexIt =
      pPrimitive->textureCoordinateMap.find(texCoordSetIndex);
  if (textureCoordinateIndexIt == pPrimitive->textureCoordinateMap.end()) {
    return -1;
  }

  return textureCoordinateIndexIt->second;
}

PRAGMA_DISABLE_DEPRECATION_WARNINGS
FString UCesiumPropertyTexturePropertyBlueprintLibrary::GetSwizzle(
    UPARAM(ref) const FCesiumPropertyTextureProperty& Property) {
  const std::string swizzle =
      callback<const std::string>(Property._property, [](const auto& view) {
        return view.getSwizzle();
      });
  return UTF8_TO_TCHAR(swizzle.c_str());
}

int64 UCesiumPropertyTexturePropertyBlueprintLibrary::GetComponentCount(
    UPARAM(ref) const FCesiumPropertyTextureProperty& Property) {
  return callback<int64>(Property._property, [](const auto& view) {
    return static_cast<int64>(view.getChannels().size());
  });
}
PRAGMA_ENABLE_DEPRECATION_WARNINGS

TArray<int64> UCesiumPropertyTexturePropertyBlueprintLibrary::GetChannels(
    UPARAM(ref) const FCesiumPropertyTextureProperty& Property) {
  return callback<TArray<int64>>(
      Property._property,
      [](const auto& view) -> TArray<int64> {
        const std::vector<int64_t>& channels = view.getChannels();
        TArray<int64> result;
        result.Reserve(static_cast<int32>(channels.size()));
        for (size_t i = 0; i < channels.size(); i++) {
          result.Emplace(channels[i]);
        }

        return result;
      });
}

uint8 UCesiumPropertyTexturePropertyBlueprintLibrary::GetByte(
    UPARAM(ref) const FCesiumPropertyTextureProperty& Property,
    const FVector2D& UV,
    uint8 DefaultValue) {
  return callback<uint8>(
      Property._property,
      [&UV, DefaultValue](const auto& view) -> uint8 {
        if (view.status() != PropertyTexturePropertyViewStatus::Valid) {
          return DefaultValue;
        }
        auto maybeValue = view.get(UV.X, UV.Y);
        if (maybeValue) {
          auto value = *maybeValue;
          return CesiumMetadataConversions<uint8, decltype(value)>::convert(
              value,
              DefaultValue);
        }
        return DefaultValue;
      });
}

int32 UCesiumPropertyTexturePropertyBlueprintLibrary::GetInteger(
    UPARAM(ref) const FCesiumPropertyTextureProperty& Property,
    const FVector2D& UV,
    int32 DefaultValue) {
  return callback<int32>(
      Property._property,
      [&UV, DefaultValue](const auto& view) -> int32 {
        if (view.status() != PropertyTexturePropertyViewStatus::Valid) {
          return DefaultValue;
        }
        auto maybeValue = view.get(UV.X, UV.Y);
        if (maybeValue) {
          auto value = *maybeValue;
          return CesiumMetadataConversions<int32, decltype(value)>::convert(
              value,
              DefaultValue);
        }
        return DefaultValue;
      });
}

float UCesiumPropertyTexturePropertyBlueprintLibrary::GetFloat(
    UPARAM(ref) const FCesiumPropertyTextureProperty& Property,
    const FVector2D& UV,
    float DefaultValue) {
  return callback<float>(
      Property._property,
      [&UV, DefaultValue](const auto& view) -> float {
        if (view.status() != PropertyTexturePropertyViewStatus::Valid) {
          return DefaultValue;
        }
        auto maybeValue = view.get(UV.X, UV.Y);
        if (maybeValue) {
          auto value = *maybeValue;
          return CesiumMetadataConversions<float, decltype(value)>::convert(
              value,
              DefaultValue);
        }
        return DefaultValue;
      });
}

double UCesiumPropertyTexturePropertyBlueprintLibrary::GetFloat64(
    UPARAM(ref) const FCesiumPropertyTextureProperty& Property,
    const FVector2D& UV,
    double DefaultValue) {
  return callback<double>(
      Property._property,
      [&UV, DefaultValue](const auto& view) -> double {
        if (view.status() != PropertyTexturePropertyViewStatus::Valid) {
          return DefaultValue;
        }
        auto maybeValue = view.get(UV.X, UV.Y);
        if (maybeValue) {
          auto value = *maybeValue;
          return CesiumMetadataConversions<double, decltype(value)>::convert(
              value,
              DefaultValue);
        }
        return DefaultValue;
      });
}

FIntPoint UCesiumPropertyTexturePropertyBlueprintLibrary::GetIntPoint(
    UPARAM(ref) const FCesiumPropertyTextureProperty& Property,
    const FVector2D& UV,
    const FIntPoint& DefaultValue) {
  return callback<FIntPoint>(
      Property._property,
      [&UV, DefaultValue](const auto& view) -> FIntPoint {
        if (view.status() != PropertyTexturePropertyViewStatus::Valid) {
          return DefaultValue;
        }
        auto maybeValue = view.get(UV.X, UV.Y);
        if (maybeValue) {
          auto value = *maybeValue;
          return CesiumMetadataConversions<FIntPoint, decltype(value)>::convert(
              value,
              DefaultValue);
        }
        return DefaultValue;
      });
}

FVector2D UCesiumPropertyTexturePropertyBlueprintLibrary::GetVector2D(
    UPARAM(ref) const FCesiumPropertyTextureProperty& Property,
    const FVector2D& UV,
    const FVector2D& DefaultValue) {
  return callback<FVector2D>(
      Property._property,
      [&UV, DefaultValue](const auto& view) -> FVector2D {
        if (view.status() != PropertyTexturePropertyViewStatus::Valid) {
          return DefaultValue;
        }
        auto maybeValue = view.get(UV.X, UV.Y);
        if (maybeValue) {
          auto value = *maybeValue;
          return CesiumMetadataConversions<FVector2D, decltype(value)>::convert(
              value,
              DefaultValue);
        }
        return DefaultValue;
      });
}

FIntVector UCesiumPropertyTexturePropertyBlueprintLibrary::GetIntVector(
    UPARAM(ref) const FCesiumPropertyTextureProperty& Property,
    const FVector2D& UV,
    const FIntVector& DefaultValue) {
  return callback<FIntVector>(
      Property._property,
      [&UV, DefaultValue](const auto& view) -> FIntVector {
        if (view.status() != PropertyTexturePropertyViewStatus::Valid) {
          return DefaultValue;
        }
        auto maybeValue = view.get(UV.X, UV.Y);
        if (maybeValue) {
          auto value = *maybeValue;
          return CesiumMetadataConversions<FIntVector, decltype(value)>::
              convert(value, DefaultValue);
        }
        return DefaultValue;
      });
}

FVector UCesiumPropertyTexturePropertyBlueprintLibrary::GetVector(
    UPARAM(ref) const FCesiumPropertyTextureProperty& Property,
    const FVector2D& UV,
    const FVector& DefaultValue) {
  return callback<FVector>(
      Property._property,
      [&UV, DefaultValue](const auto& view) -> FVector {
        if (view.status() != PropertyTexturePropertyViewStatus::Valid) {
          return DefaultValue;
        }
        auto maybeValue = view.get(UV.X, UV.Y);
        if (maybeValue) {
          auto value = *maybeValue;
          return CesiumMetadataConversions<FVector, decltype(value)>::convert(
              value,
              DefaultValue);
        }
        return DefaultValue;
      });
}

FVector4 UCesiumPropertyTexturePropertyBlueprintLibrary::GetVector4(
    UPARAM(ref) const FCesiumPropertyTextureProperty& Property,
    const FVector2D& UV,
    const FVector4& DefaultValue) {
  return callback<FVector4>(
      Property._property,
      [&UV, DefaultValue](const auto& view) -> FVector4 {
        if (view.status() != PropertyTexturePropertyViewStatus::Valid) {
          return DefaultValue;
        }
        auto maybeValue = view.get(UV.X, UV.Y);
        if (maybeValue) {
          auto value = *maybeValue;
          return CesiumMetadataConversions<FVector4, decltype(value)>::convert(
              value,
              DefaultValue);
        }
        return DefaultValue;
      });
}

FCesiumPropertyArray UCesiumPropertyTexturePropertyBlueprintLibrary::GetArray(
    UPARAM(ref) const FCesiumPropertyTextureProperty& Property,
    const FVector2D& UV) {
  return callback<FCesiumPropertyArray>(
      Property._property,
      [&UV](const auto& view) -> FCesiumPropertyArray {
        if (view.status() != PropertyTexturePropertyViewStatus::Valid) {
          return FCesiumPropertyArray();
        }
        auto maybeValue = view.get(UV.X, UV.Y);
        if (maybeValue) {
          auto value = *maybeValue;
          if constexpr (CesiumGltf::IsMetadataArray<decltype(value)>::value) {
            return FCesiumPropertyArray(value);
          }
        }
        return FCesiumPropertyArray();
      });
}

FCesiumMetadataValue UCesiumPropertyTexturePropertyBlueprintLibrary::GetValue(
    UPARAM(ref) const FCesiumPropertyTextureProperty& Property,
    const FVector2D& UV) {
  return callback<FCesiumMetadataValue>(
      Property._property,
      [&UV](const auto& view) -> FCesiumMetadataValue {
        if (view.status() != PropertyTexturePropertyViewStatus::Valid &&
            view.status() !=
                PropertyTexturePropertyViewStatus::EmptyPropertyWithDefault) {
          return FCesiumMetadataValue();
        }

        return FCesiumMetadataValue(view.get(UV.X, UV.Y));
      });
}

FCesiumMetadataValue
UCesiumPropertyTexturePropertyBlueprintLibrary::GetRawValue(
    UPARAM(ref) const FCesiumPropertyTextureProperty& Property,
    const FVector2D& UV) {
  return callback<FCesiumMetadataValue>(
      Property._property,
      [&UV](const auto& view) -> FCesiumMetadataValue {
        if (view.status() != PropertyTexturePropertyViewStatus::Valid) {
          return FCesiumMetadataValue();
        }

        return FCesiumMetadataValue(view.getRaw(UV.X, UV.Y));
      });
}

bool UCesiumPropertyTexturePropertyBlueprintLibrary::IsNormalized(
    UPARAM(ref) const FCesiumPropertyTextureProperty& Property) {
  return Property._normalized;
}

FCesiumMetadataValue UCesiumPropertyTexturePropertyBlueprintLibrary::GetOffset(
    UPARAM(ref) const FCesiumPropertyTextureProperty& Property) {
  return callback<FCesiumMetadataValue>(
      Property._property,
      [](const auto& view) -> FCesiumMetadataValue {
        // Returns an empty value if no offset is specified.
        return FCesiumMetadataValue(view.offset());
      });
}

FCesiumMetadataValue UCesiumPropertyTexturePropertyBlueprintLibrary::GetScale(
    UPARAM(ref) const FCesiumPropertyTextureProperty& Property) {
  return callback<FCesiumMetadataValue>(
      Property._property,
      [](const auto& view) -> FCesiumMetadataValue {
        // Returns an empty value if no scale is specified.
        return FCesiumMetadataValue(view.scale());
      });
}

FCesiumMetadataValue
UCesiumPropertyTexturePropertyBlueprintLibrary::GetMinimumValue(
    UPARAM(ref) const FCesiumPropertyTextureProperty& Property) {
  return callback<FCesiumMetadataValue>(
      Property._property,
      [](const auto& view) -> FCesiumMetadataValue {
        // Returns an empty value if no min is specified.
        return FCesiumMetadataValue(view.min());
      });
}

FCesiumMetadataValue
UCesiumPropertyTexturePropertyBlueprintLibrary::GetMaximumValue(
    UPARAM(ref) const FCesiumPropertyTextureProperty& Property) {
  return callback<FCesiumMetadataValue>(
      Property._property,
      [](const auto& view) -> FCesiumMetadataValue {
        // Returns an empty value if no max is specified.
        return FCesiumMetadataValue(view.max());
      });
}

FCesiumMetadataValue
UCesiumPropertyTexturePropertyBlueprintLibrary::GetNoDataValue(
    UPARAM(ref) const FCesiumPropertyTextureProperty& Property) {
  return callback<FCesiumMetadataValue>(
      Property._property,
      [](const auto& view) -> FCesiumMetadataValue {
        // Returns an empty value if no "no data" value is specified.
        return FCesiumMetadataValue(view.noData());
      });
}

FCesiumMetadataValue
UCesiumPropertyTexturePropertyBlueprintLibrary::GetDefaultValue(
    UPARAM(ref) const FCesiumPropertyTextureProperty& Property) {
  return callback<FCesiumMetadataValue>(
      Property._property,
      [](const auto& view) -> FCesiumMetadataValue {
        // Returns an empty value if no default value is specified.
        return FCesiumMetadataValue(view.defaultValue());
      });
}
