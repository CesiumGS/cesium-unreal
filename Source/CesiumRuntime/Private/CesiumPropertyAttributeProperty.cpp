// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumPropertyAttributeProperty.h"
#include "CesiumMetadataEnum.h"
#include "UnrealMetadataConversions.h"

#include <CesiumGltf/MetadataConversions.h>
#include <CesiumGltf/PropertyTypeTraits.h>
#include <utility>

namespace {
/**
 * Callback on a std::any, assuming that it contains a
 * PropertyAttributePropertyView of the specified type. If the type does not
 * match, the callback is performed on an invalid PropertyAttributePropertyView
 * instead.
 *
 * @param property The std::any containing the property.
 * @param callback The callback function.
 *
 * @tparam TProperty The property type.
 * @tparam Normalized Whether the PropertyAttributePropertyView is normalized.
 * @tparam TResult The type of the output from the callback function.
 * @tparam Callback The callback function type.
 */
template <
    typename TProperty,
    bool Normalized,
    typename TResult,
    typename Callback>
TResult propertyAttributePropertyCallback(
    const std::any& property,
    Callback&& callback) {
  const CesiumGltf::PropertyAttributePropertyView<TProperty, Normalized>*
      pProperty = std::any_cast<
          CesiumGltf::PropertyAttributePropertyView<TProperty, Normalized>>(
          &property);
  if (pProperty) {
    return callback(*pProperty);
  }

  return callback(CesiumGltf::PropertyAttributePropertyView<uint8_t>());
}

/**
 * Callback on a std::any, assuming that it contains a
 * PropertyAttributePropertyView on a scalar type. If the valueType does not
 * have a valid component type, the callback is performed on an invalid
 * PropertyAttributePropertyView instead.
 *
 * @param property The std::any containing the property.
 * @param valueType The FCesiumMetadataValueType of the property.
 * @param callback The callback function.
 *
 * @tparam Normalized Whether the PropertyAttributePropertyView is normalized.
 * @tparam TResult The type of the output from the callback function.
 * @tparam Callback The callback function type.
 */
template <bool Normalized, typename TResult, typename Callback>
TResult scalarPropertyAttributePropertyCallback(
    const std::any& property,
    const FCesiumMetadataValueType& valueType,
    Callback&& callback) {
  switch (valueType.ComponentType) {
  case ECesiumMetadataComponentType::Int8:
    return propertyAttributePropertyCallback<
        int8_t,
        Normalized,
        TResult,
        Callback>(property, std::forward<Callback>(callback));
  case ECesiumMetadataComponentType::Uint8:
    return propertyAttributePropertyCallback<
        uint8_t,
        Normalized,
        TResult,
        Callback>(property, std::forward<Callback>(callback));
  case ECesiumMetadataComponentType::Int16:
    return propertyAttributePropertyCallback<
        int16_t,
        Normalized,
        TResult,
        Callback>(property, std::forward<Callback>(callback));
  case ECesiumMetadataComponentType::Uint16:
    return propertyAttributePropertyCallback<
        uint16_t,
        Normalized,
        TResult,
        Callback>(property, std::forward<Callback>(callback));
  case ECesiumMetadataComponentType::Uint32:
    return propertyAttributePropertyCallback<
        uint32_t,
        Normalized,
        TResult,
        Callback>(property, std::forward<Callback>(callback));
  case ECesiumMetadataComponentType::Float32:
    return propertyAttributePropertyCallback<float, false, TResult, Callback>(
        property,
        std::forward<Callback>(callback));
  default:
    return callback(CesiumGltf::PropertyAttributePropertyView<uint8_t>());
  }
}

/**
 * Callback on a std::any, assuming that it contains a
 * PropertyAttributePropertyView on a glm::vecN type. If the valueType does not
 * have a valid component type, the callback is performed on an invalid
 * PropertyAttributePropertyView instead.
 *
 * @param property The std::any containing the property.
 * @param valueType The FCesiumMetadataValueType of the property.
 * @param callback The callback function.
 *
 * @tparam N The dimensions of the glm::vecN
 * @tparam Normalized Whether the PropertyAttributePropertyView is normalized.
 * @tparam TResult The type of the output from the callback function.
 * @tparam Callback The callback function type.
 */
template <glm::length_t N, bool Normalized, typename TResult, typename Callback>
TResult vecNPropertyAttributePropertyCallback(
    const std::any& property,
    const FCesiumMetadataValueType& valueType,
    Callback&& callback) {
  switch (valueType.ComponentType) {
  case ECesiumMetadataComponentType::Int8:
    return propertyAttributePropertyCallback<
        glm::vec<N, int8_t>,
        Normalized,
        TResult,
        Callback>(property, std::forward<Callback>(callback));
  case ECesiumMetadataComponentType::Uint8:
    return propertyAttributePropertyCallback<
        glm::vec<N, uint8_t>,
        Normalized,
        TResult,
        Callback>(property, std::forward<Callback>(callback));
  case ECesiumMetadataComponentType::Int16:
    return propertyAttributePropertyCallback<
        glm::vec<N, int16_t>,
        Normalized,
        TResult,
        Callback>(property, std::forward<Callback>(callback));
  case ECesiumMetadataComponentType::Uint16:
    return propertyAttributePropertyCallback<
        glm::vec<N, uint16_t>,
        Normalized,
        TResult,
        Callback>(property, std::forward<Callback>(callback));
  case ECesiumMetadataComponentType::Uint32:
    return propertyAttributePropertyCallback<
        glm::vec<N, uint32_t>,
        Normalized,
        TResult,
        Callback>(property, std::forward<Callback>(callback));
  case ECesiumMetadataComponentType::Float32:
    return propertyAttributePropertyCallback<
        glm::vec<N, float>,
        false,
        TResult,
        Callback>(property, std::forward<Callback>(callback));
  default:
    return callback(CesiumGltf::PropertyAttributePropertyView<uint8_t>());
  }
}

/**
 * Callback on a std::any, assuming that it contains a
 * PropertyAttributePropertyView on a glm::vecN type. If the valueType does not
 * have a valid component type, the callback is performed on an invalid
 * PropertyAttributePropertyView instead.
 *
 * @param property The std::any containing the property.
 * @param valueType The FCesiumMetadataValueType of the property.
 * @param callback The callback function.
 *
 * @tparam Normalized Whether the PropertyAttributePropertyView is normalized.
 * @tparam TResult The type of the output from the callback function.
 * @tparam Callback The callback function type.
 */
template <bool Normalized, typename TResult, typename Callback>
TResult vecNPropertyAttributePropertyCallback(
    const std::any& property,
    const FCesiumMetadataValueType& valueType,
    Callback&& callback) {
  switch (valueType.Type) {
  case ECesiumMetadataType::Vec2:
    return vecNPropertyAttributePropertyCallback<
        2,
        Normalized,
        TResult,
        Callback>(property, valueType, std::forward<Callback>(callback));
  case ECesiumMetadataType::Vec3:
    return vecNPropertyAttributePropertyCallback<
        3,
        Normalized,
        TResult,
        Callback>(property, valueType, std::forward<Callback>(callback));
  case ECesiumMetadataType::Vec4:
    return vecNPropertyAttributePropertyCallback<
        4,
        Normalized,
        TResult,
        Callback>(property, valueType, std::forward<Callback>(callback));
  default:
    return callback(CesiumGltf::PropertyAttributePropertyView<uint8_t>());
  }
}

/**
 * Callback on a std::any, assuming that it contains a
 * PropertyAttributePropertyView on a glm::matN type. If the valueType does not
 * have a valid component type, the callback is performed on an invalid
 * PropertyAttributePropertyView instead.
 *
 * @param property The std::any containing the property.
 * @param valueType The FCesiumMetadataValueType of the property.
 * @param callback The callback function.
 *
 * @tparam N The dimensions of the glm::matN
 * @tparam TNormalized Whether the PropertyAttributePropertyView is normalized.
 * @tparam TResult The type of the output from the callback function.
 * @tparam Callback The callback function type.
 */
template <glm::length_t N, bool Normalized, typename TResult, typename Callback>
TResult matNPropertyAttributePropertyCallback(
    const std::any& property,
    const FCesiumMetadataValueType& valueType,
    Callback&& callback) {
  switch (valueType.ComponentType) {
  case ECesiumMetadataComponentType::Int8:
    return propertyAttributePropertyCallback<
        glm::mat<N, N, int8_t>,
        Normalized,
        TResult,
        Callback>(property, std::forward<Callback>(callback));
  case ECesiumMetadataComponentType::Uint8:
    return propertyAttributePropertyCallback<
        glm::mat<N, N, uint8_t>,
        Normalized,
        TResult,
        Callback>(property, std::forward<Callback>(callback));
  case ECesiumMetadataComponentType::Int16:
    return propertyAttributePropertyCallback<
        glm::mat<N, N, int16_t>,
        Normalized,
        TResult,
        Callback>(property, std::forward<Callback>(callback));
  case ECesiumMetadataComponentType::Uint16:
    return propertyAttributePropertyCallback<
        glm::mat<N, N, uint16_t>,
        Normalized,
        TResult,
        Callback>(property, std::forward<Callback>(callback));
  case ECesiumMetadataComponentType::Uint32:
    return propertyAttributePropertyCallback<
        glm::mat<N, N, uint32_t>,
        Normalized,
        TResult,
        Callback>(property, std::forward<Callback>(callback));
  case ECesiumMetadataComponentType::Float32:
    return propertyAttributePropertyCallback<
        glm::mat<N, N, float>,
        false,
        TResult,
        Callback>(property, std::forward<Callback>(callback));
  default:
    return callback(CesiumGltf::PropertyAttributePropertyView<uint8_t>());
  }
}

/**
 * Callback on a std::any, assuming that it contains a
 * PropertyAttributePropertyView on a glm::matN type. If the valueType does not
 * have a valid component type, the callback is performed on an invalid
 * PropertyAttributePropertyView instead.
 *
 * @param property The std::any containing the property.
 * @param valueType The FCesiumMetadataValueType of the property.
 * @param callback The callback function.
 *
 * @tparam Normalized Whether the PropertyAttributePropertyView is normalized.
 * @tparam TResult The type of the output from the callback function.
 * @tparam Callback The callback function type.
 */
template <bool Normalized, typename TResult, typename Callback>
TResult matNPropertyAttributePropertyCallback(
    const std::any& property,
    const FCesiumMetadataValueType& valueType,
    Callback&& callback) {
  if (valueType.Type == ECesiumMetadataType::Mat2) {
    return matNPropertyAttributePropertyCallback<
        2,
        Normalized,
        TResult,
        Callback>(property, valueType, std::forward<Callback>(callback));
  }

  if (valueType.Type == ECesiumMetadataType::Mat3) {
    return matNPropertyAttributePropertyCallback<
        3,
        Normalized,
        TResult,
        Callback>(property, valueType, std::forward<Callback>(callback));
  }

  if (valueType.Type == ECesiumMetadataType::Mat4) {
    return matNPropertyAttributePropertyCallback<
        4,
        Normalized,
        TResult,
        Callback>(property, valueType, std::forward<Callback>(callback));
  }

  return callback(CesiumGltf::PropertyAttributePropertyView<uint8>());
}

template <typename TResult, typename Callback>
TResult propertyAttributePropertyCallback(
    const std::any& property,
    const FCesiumMetadataValueType& valueType,
    bool normalized,
    Callback&& callback) {
  if (valueType.bIsArray) {
    // Array types are not supported for property attribute properties.
    return callback(CesiumGltf::PropertyAttributePropertyView<uint8_t>());
  }

  switch (valueType.Type) {
  case ECesiumMetadataType::Scalar:
    return normalized ? scalarPropertyAttributePropertyCallback<
                            true,
                            TResult,
                            Callback>(
                            property,
                            valueType,
                            std::forward<Callback>(callback))
                      : scalarPropertyAttributePropertyCallback<
                            false,
                            TResult,
                            Callback>(
                            property,
                            valueType,
                            std::forward<Callback>(callback));
  case ECesiumMetadataType::Enum:
    return scalarPropertyAttributePropertyCallback<false, TResult, Callback>(
        property,
        valueType,
        std::forward<Callback>(callback));
  case ECesiumMetadataType::Vec2:
  case ECesiumMetadataType::Vec3:
  case ECesiumMetadataType::Vec4:
    return normalized
               ? vecNPropertyAttributePropertyCallback<true, TResult, Callback>(
                     property,
                     valueType,
                     std::forward<Callback>(callback))
               : vecNPropertyAttributePropertyCallback<
                     false,
                     TResult,
                     Callback>(
                     property,
                     valueType,
                     std::forward<Callback>(callback));
  case ECesiumMetadataType::Mat2:
  case ECesiumMetadataType::Mat3:
  case ECesiumMetadataType::Mat4:
    return normalized
               ? matNPropertyAttributePropertyCallback<true, TResult, Callback>(
                     property,
                     valueType,
                     std::forward<Callback>(callback))
               : matNPropertyAttributePropertyCallback<
                     false,
                     TResult,
                     Callback>(
                     property,
                     valueType,
                     std::forward<Callback>(callback));
  default:
    return callback(CesiumGltf::PropertyAttributePropertyView<uint8_t>());
  }
}

} // namespace

int64 FCesiumPropertyAttributeProperty::getAccessorStride() const {
  return propertyAttributePropertyCallback<int64>(
      this->_property,
      this->_valueType,
      this->_normalized,
      [](const auto& view) -> int64 {
        return view.accessorView().stride();
      });
}

const std::byte* FCesiumPropertyAttributeProperty::getAccessorData() const {
  return propertyAttributePropertyCallback<const std::byte*>(
      this->_property,
      this->_valueType,
      this->_normalized,
      [](const auto& view) -> const std::byte* {
        return view.accessorView().data();
      });
}

ECesiumPropertyAttributePropertyStatus
UCesiumPropertyAttributePropertyBlueprintLibrary::
    GetPropertyAttributePropertyStatus(
        UPARAM(ref) const FCesiumPropertyAttributeProperty& Property) {
  return Property._status;
}

ECesiumMetadataBlueprintType
UCesiumPropertyAttributePropertyBlueprintLibrary::GetBlueprintType(
    UPARAM(ref) const FCesiumPropertyAttributeProperty& Property) {
  return CesiumMetadataValueTypeToBlueprintType(Property._valueType);
}

FCesiumMetadataValueType
UCesiumPropertyAttributePropertyBlueprintLibrary::GetValueType(
    UPARAM(ref) const FCesiumPropertyAttributeProperty& Property) {
  return Property._valueType;
}

int64 UCesiumPropertyAttributePropertyBlueprintLibrary::GetPropertySize(
    UPARAM(ref) const FCesiumPropertyAttributeProperty& Property) {
  return propertyAttributePropertyCallback<int64>(
      Property._property,
      Property._valueType,
      Property._normalized,
      [](const auto& view) -> int64 { return view.size(); });
}

uint8 UCesiumPropertyAttributePropertyBlueprintLibrary::GetByte(
    UPARAM(ref) const FCesiumPropertyAttributeProperty& Property,
    int64 Index,
    uint8 DefaultValue) {
  return propertyAttributePropertyCallback<uint8_t>(
      Property._property,
      Property._valueType,
      Property._normalized,
      [Index, DefaultValue](const auto& v) -> uint8 {
        // size() returns zero if the view is invalid.
        if (Index < 0 || Index >= v.size()) {
          return DefaultValue;
        }
        auto maybeValue = v.get(Index);
        if (maybeValue) {
          auto value = *maybeValue;
          return CesiumGltf::MetadataConversions<uint8, decltype(value)>::
              convert(value)
                  .value_or(DefaultValue);
        }
        return DefaultValue;
      });
}

int32 UCesiumPropertyAttributePropertyBlueprintLibrary::GetInteger(
    UPARAM(ref) const FCesiumPropertyAttributeProperty& Property,
    int64 Index,
    int32 DefaultValue) {
  return propertyAttributePropertyCallback<int32>(
      Property._property,
      Property._valueType,
      Property._normalized,
      [Index, DefaultValue](const auto& v) -> int32 {
        // size() returns zero if the view is invalid.
        if (Index < 0 || Index >= v.size()) {
          return DefaultValue;
        }
        auto maybeValue = v.get(Index);
        if (maybeValue) {
          auto value = *maybeValue;
          return CesiumGltf::MetadataConversions<int32, decltype(value)>::
              convert(value)
                  .value_or(DefaultValue);
        }
        return DefaultValue;
      });
}

int64 UCesiumPropertyAttributePropertyBlueprintLibrary::GetInteger64(
    UPARAM(ref) const FCesiumPropertyAttributeProperty& Property,
    int64 Index,
    int64 DefaultValue) {
  return propertyAttributePropertyCallback<int64>(
      Property._property,
      Property._valueType,
      Property._normalized,
      [Index, DefaultValue](const auto& v) -> int64 {
        // size() returns zero if the view is invalid.
        if (Index < 0 || Index >= v.size()) {
          return DefaultValue;
        }
        auto maybeValue = v.get(Index);
        if (maybeValue) {
          auto value = *maybeValue;
          return CesiumGltf::MetadataConversions<int64, decltype(value)>::
              convert(value)
                  .value_or(DefaultValue);
        }
        return DefaultValue;
      });
}

float UCesiumPropertyAttributePropertyBlueprintLibrary::GetFloat(
    UPARAM(ref) const FCesiumPropertyAttributeProperty& Property,
    int64 Index,
    float DefaultValue) {
  return propertyAttributePropertyCallback<float>(
      Property._property,
      Property._valueType,
      Property._normalized,
      [Index, DefaultValue](const auto& v) -> float {
        // size() returns zero if the view is invalid.
        if (Index < 0 || Index >= v.size()) {
          return DefaultValue;
        }
        auto maybeValue = v.get(Index);
        if (maybeValue) {
          auto value = *maybeValue;
          return CesiumGltf::MetadataConversions<float, decltype(value)>::
              convert(value)
                  .value_or(DefaultValue);
        }
        return DefaultValue;
      });
}

double UCesiumPropertyAttributePropertyBlueprintLibrary::GetFloat64(
    UPARAM(ref) const FCesiumPropertyAttributeProperty& Property,
    int64 Index,
    double DefaultValue) {
  return propertyAttributePropertyCallback<double>(
      Property._property,
      Property._valueType,
      Property._normalized,
      [Index, DefaultValue](const auto& v) -> double {
        // size() returns zero if the view is invalid.
        if (Index < 0 || Index >= v.size()) {
          return DefaultValue;
        }
        auto maybeValue = v.get(Index);
        if (maybeValue) {
          auto value = *maybeValue;
          return CesiumGltf::MetadataConversions<double, decltype(value)>::
              convert(value)
                  .value_or(DefaultValue);
        }
        return DefaultValue;
      });
}

FIntPoint UCesiumPropertyAttributePropertyBlueprintLibrary::GetIntPoint(
    UPARAM(ref) const FCesiumPropertyAttributeProperty& Property,
    int64 Index,
    const FIntPoint& DefaultValue) {
  return propertyAttributePropertyCallback<FIntPoint>(
      Property._property,
      Property._valueType,
      Property._normalized,
      [Index, &DefaultValue](const auto& v) -> FIntPoint {
        // size() returns zero if the view is invalid.
        if (Index < 0 || Index >= v.size()) {
          return DefaultValue;
        }
        auto maybeValue = v.get(Index);
        if (!maybeValue) {
          return DefaultValue;
        }

        auto value = *maybeValue;
        if constexpr (CesiumGltf::IsMetadataString<decltype(value)>::value) {
          return UnrealMetadataConversions::toIntPoint(value, DefaultValue);
        } else {
          auto maybeVec2 = CesiumGltf::
              MetadataConversions<glm::ivec2, decltype(value)>::convert(value);
          return maybeVec2 ? UnrealMetadataConversions::toIntPoint(*maybeVec2)
                           : DefaultValue;
        }
      });
}

FVector2D UCesiumPropertyAttributePropertyBlueprintLibrary::GetVector2D(
    UPARAM(ref) const FCesiumPropertyAttributeProperty& Property,
    int64 Index,
    const FVector2D& DefaultValue) {
  return propertyAttributePropertyCallback<FVector2D>(
      Property._property,
      Property._valueType,
      Property._normalized,
      [Index, &DefaultValue](const auto& v) -> FVector2D {
        // size() returns zero if the view is invalid.
        if (Index < 0 || Index >= v.size()) {
          return DefaultValue;
        }
        auto maybeValue = v.get(Index);
        if (!maybeValue) {
          return DefaultValue;
        }

        auto value = *maybeValue;
        if constexpr (CesiumGltf::IsMetadataString<decltype(value)>::value) {
          return UnrealMetadataConversions::toVector2D(value, DefaultValue);
        } else {
          auto maybeVec2 = CesiumGltf::
              MetadataConversions<glm::dvec2, decltype(value)>::convert(value);
          return maybeVec2 ? UnrealMetadataConversions::toVector2D(*maybeVec2)
                           : DefaultValue;
        }
      });
}

FIntVector UCesiumPropertyAttributePropertyBlueprintLibrary::GetIntVector(
    UPARAM(ref) const FCesiumPropertyAttributeProperty& Property,
    int64 Index,
    const FIntVector& DefaultValue) {
  return propertyAttributePropertyCallback<FIntVector>(
      Property._property,
      Property._valueType,
      Property._normalized,
      [Index, &DefaultValue](const auto& v) -> FIntVector {
        // size() returns zero if the view is invalid.
        if (Index < 0 || Index >= v.size()) {
          return DefaultValue;
        }
        auto maybeValue = v.get(Index);
        if (!maybeValue) {
          return DefaultValue;
        }

        auto value = *maybeValue;
        if constexpr (CesiumGltf::IsMetadataString<decltype(value)>::value) {
          return UnrealMetadataConversions::toIntVector(value, DefaultValue);
        } else {
          auto maybeVec3 = CesiumGltf::
              MetadataConversions<glm::ivec3, decltype(value)>::convert(value);
          return maybeVec3 ? UnrealMetadataConversions::toIntVector(*maybeVec3)
                           : DefaultValue;
        }
      });
}

FVector3f UCesiumPropertyAttributePropertyBlueprintLibrary::GetVector3f(
    UPARAM(ref) const FCesiumPropertyAttributeProperty& Property,
    int64 Index,
    const FVector3f& DefaultValue) {
  return propertyAttributePropertyCallback<FVector3f>(
      Property._property,
      Property._valueType,
      Property._normalized,
      [Index, &DefaultValue](const auto& v) -> FVector3f {
        // size() returns zero if the view is invalid.
        if (Index < 0 || Index >= v.size()) {
          return DefaultValue;
        }
        auto maybeValue = v.get(Index);
        if (!maybeValue) {
          return DefaultValue;
        }

        auto value = *maybeValue;
        if constexpr (CesiumGltf::IsMetadataString<decltype(value)>::value) {
          return UnrealMetadataConversions::toVector3f(value, DefaultValue);
        } else {
          auto maybeVec3 = CesiumGltf::
              MetadataConversions<glm::vec3, decltype(value)>::convert(value);
          return maybeVec3 ? UnrealMetadataConversions::toVector3f(*maybeVec3)
                           : DefaultValue;
        }
      });
}

FVector UCesiumPropertyAttributePropertyBlueprintLibrary::GetVector(
    UPARAM(ref) const FCesiumPropertyAttributeProperty& Property,
    int64 Index,
    const FVector& DefaultValue) {
  return propertyAttributePropertyCallback<FVector>(
      Property._property,
      Property._valueType,
      Property._normalized,
      [Index, &DefaultValue](const auto& v) -> FVector {
        // size() returns zero if the view is invalid.
        if (Index < 0 || Index >= v.size()) {
          return DefaultValue;
        }
        auto maybeValue = v.get(Index);
        if (!maybeValue) {
          return DefaultValue;
        }

        auto value = *maybeValue;
        if constexpr (CesiumGltf::IsMetadataString<decltype(value)>::value) {
          return UnrealMetadataConversions::toVector(value, DefaultValue);
        } else {
          auto maybeVec3 = CesiumGltf::
              MetadataConversions<glm::dvec3, decltype(value)>::convert(value);
          return maybeVec3 ? UnrealMetadataConversions::toVector(*maybeVec3)
                           : DefaultValue;
        }
      });
}

FVector4 UCesiumPropertyAttributePropertyBlueprintLibrary::GetVector4(
    UPARAM(ref) const FCesiumPropertyAttributeProperty& Property,
    int64 Index,
    const FVector4& DefaultValue) {
  return propertyAttributePropertyCallback<FVector4>(
      Property._property,
      Property._valueType,
      Property._normalized,
      [Index, &DefaultValue](const auto& v) -> FVector4 {
        // size() returns zero if the view is invalid.
        if (Index < 0 || Index >= v.size()) {
          return DefaultValue;
        }
        auto maybeValue = v.get(Index);
        if (!maybeValue) {
          return DefaultValue;
        }

        auto value = *maybeValue;
        if constexpr (CesiumGltf::IsMetadataString<decltype(value)>::value) {
          return UnrealMetadataConversions::toVector4(value, DefaultValue);
        } else {
          auto maybeVec4 = CesiumGltf::
              MetadataConversions<glm::dvec4, decltype(value)>::convert(value);
          return maybeVec4 ? UnrealMetadataConversions::toVector4(*maybeVec4)
                           : DefaultValue;
        }
      });
}

FMatrix UCesiumPropertyAttributePropertyBlueprintLibrary::GetMatrix(
    UPARAM(ref) const FCesiumPropertyAttributeProperty& Property,
    int64 Index,
    const FMatrix& DefaultValue) {
  auto maybeMat4 = propertyAttributePropertyCallback<std::optional<glm::dmat4>>(
      Property._property,
      Property._valueType,
      Property._normalized,
      [Index](const auto& v) -> std::optional<glm::dmat4> {
        // size() returns zero if the view is invalid.
        if (Index < 0 || Index >= v.size()) {
          return std::nullopt;
        }
        auto maybeValue = v.get(Index);
        if (!maybeValue) {
          return std::nullopt;
        }

        auto value = *maybeValue;
        return CesiumGltf::MetadataConversions<glm::dmat4, decltype(value)>::
            convert(value);
      });

  return maybeMat4 ? UnrealMetadataConversions::toMatrix(*maybeMat4)
                   : DefaultValue;
}

FCesiumMetadataValue UCesiumPropertyAttributePropertyBlueprintLibrary::GetValue(
    UPARAM(ref) const FCesiumPropertyAttributeProperty& Property,
    int64 Index) {
  return propertyAttributePropertyCallback<FCesiumMetadataValue>(
      Property._property,
      Property._valueType,
      Property._normalized,
      [Index, &pEnumDefinition = Property._pEnumDefinition](
          const auto& view) -> FCesiumMetadataValue {
        // size() returns zero if the view is invalid.
        if (Index >= 0 && Index < view.size()) {
          return FCesiumMetadataValue(view.get(Index), pEnumDefinition);
        }
        return FCesiumMetadataValue();
      });
}

FCesiumMetadataValue
UCesiumPropertyAttributePropertyBlueprintLibrary::GetRawValue(
    UPARAM(ref) const FCesiumPropertyAttributeProperty& Property,
    int64 Index) {
  return propertyAttributePropertyCallback<FCesiumMetadataValue>(
      Property._property,
      Property._valueType,
      Property._normalized,
      [Index, &pEnumDefinition = Property._pEnumDefinition](
          const auto& view) -> FCesiumMetadataValue {
        // Return an empty value if the property is empty.
        if (view.status() == CesiumGltf::PropertyAttributePropertyViewStatus::
                                 EmptyPropertyWithDefault) {
          return FCesiumMetadataValue();
        }

        // size() returns zero if the view is invalid.
        if (Index >= 0 && Index < view.size()) {
          return FCesiumMetadataValue(
              CesiumGltf::propertyValueViewToCopy(view.getRaw(Index)),
              pEnumDefinition);
        }

        return FCesiumMetadataValue();
      });
}

bool UCesiumPropertyAttributePropertyBlueprintLibrary::IsNormalized(
    UPARAM(ref) const FCesiumPropertyAttributeProperty& Property) {
  return Property._normalized;
}

FCesiumMetadataValue
UCesiumPropertyAttributePropertyBlueprintLibrary::GetOffset(
    UPARAM(ref) const FCesiumPropertyAttributeProperty& Property) {
  return propertyAttributePropertyCallback<FCesiumMetadataValue>(
      Property._property,
      Property._valueType,
      Property._normalized,
      [](const auto& view) -> FCesiumMetadataValue {
        // Returns an empty value if no offset is specified.
        return FCesiumMetadataValue(
            CesiumGltf::propertyValueViewToCopy(view.offset()));
      });
}

FCesiumMetadataValue UCesiumPropertyAttributePropertyBlueprintLibrary::GetScale(
    UPARAM(ref) const FCesiumPropertyAttributeProperty& Property) {
  return propertyAttributePropertyCallback<FCesiumMetadataValue>(
      Property._property,
      Property._valueType,
      Property._normalized,
      [](const auto& view) -> FCesiumMetadataValue {
        // Returns an empty value if no scale is specified.
        return FCesiumMetadataValue(
            CesiumGltf::propertyValueViewToCopy(view.scale()));
      });
}

FCesiumMetadataValue
UCesiumPropertyAttributePropertyBlueprintLibrary::GetMinimumValue(
    UPARAM(ref) const FCesiumPropertyAttributeProperty& Property) {
  return propertyAttributePropertyCallback<FCesiumMetadataValue>(
      Property._property,
      Property._valueType,
      Property._normalized,
      [](const auto& view) -> FCesiumMetadataValue {
        // Returns an empty value if no min is specified.
        return FCesiumMetadataValue(
            CesiumGltf::propertyValueViewToCopy(view.min()));
      });
}

FCesiumMetadataValue
UCesiumPropertyAttributePropertyBlueprintLibrary::GetMaximumValue(
    UPARAM(ref) const FCesiumPropertyAttributeProperty& Property) {
  return propertyAttributePropertyCallback<FCesiumMetadataValue>(
      Property._property,
      Property._valueType,
      Property._normalized,
      [](const auto& view) -> FCesiumMetadataValue {
        // Returns an empty value if no max is specified.
        return FCesiumMetadataValue(
            CesiumGltf::propertyValueViewToCopy(view.max()));
      });
}

FCesiumMetadataValue
UCesiumPropertyAttributePropertyBlueprintLibrary::GetNoDataValue(
    UPARAM(ref) const FCesiumPropertyAttributeProperty& Property) {
  return propertyAttributePropertyCallback<FCesiumMetadataValue>(
      Property._property,
      Property._valueType,
      Property._normalized,
      [](const auto& view) -> FCesiumMetadataValue {
        // Returns an empty value if no "no data" value is specified.
        return FCesiumMetadataValue(
            CesiumGltf::propertyValueViewToCopy(view.noData()));
      });
}

FCesiumMetadataValue
UCesiumPropertyAttributePropertyBlueprintLibrary::GetDefaultValue(
    UPARAM(ref) const FCesiumPropertyAttributeProperty& Property) {
  return propertyAttributePropertyCallback<FCesiumMetadataValue>(
      Property._property,
      Property._valueType,
      Property._normalized,
      [](const auto& view) -> FCesiumMetadataValue {
        // Returns an empty value if no default value is specified.
        return FCesiumMetadataValue(
            CesiumGltf::propertyValueViewToCopy(view.defaultValue()));
      });
}
