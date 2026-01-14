// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#include "CesiumMetadataValue.h"
#include "CesiumPropertyArray.h"
#include "CesiumPropertyArrayBlueprintLibrary.h"
#include "UnrealMetadataConversions.h"
#include <CesiumGltf/MetadataConversions.h>
#include <CesiumGltf/PropertyTypeTraits.h>

static FCesiumMetadataValue EmptyMetadataValue = FCesiumMetadataValue();

FCesiumMetadataValue::FCesiumMetadataValue()
    : _value(), _arrayValue(), _valueType(), _pEnumDefinition(nullptr) {}

FCesiumMetadataValue::FCesiumMetadataValue(FCesiumPropertyArray&& Array)
    : _value(),
      _arrayValue(),
      _valueType(),
      _pEnumDefinition(Array._pEnumDefinition) {
  FCesiumMetadataValueType elementType =
      UCesiumPropertyArrayBlueprintLibrary::GetElementValueType(Array);
  this->_valueType = {elementType.Type, elementType.ComponentType, true};
  this->_arrayValue = std::move(Array);
}

FCesiumMetadataValue::FCesiumMetadataValue(FCesiumMetadataValue&& rhs) =
    default;

FCesiumMetadataValue&
FCesiumMetadataValue::operator=(FCesiumMetadataValue&& rhs) = default;

FCesiumMetadataValue::FCesiumMetadataValue(const FCesiumMetadataValue& rhs)
    : _value(),
      _arrayValue(rhs._arrayValue),
      _valueType(rhs._valueType),
      _pEnumDefinition(rhs._pEnumDefinition) {
  swl::visit([this](const auto& value) { this->_value = value; }, rhs._value);
}

FCesiumMetadataValue&
FCesiumMetadataValue::operator=(const FCesiumMetadataValue& rhs) {
  *this = FCesiumMetadataValue(rhs);
  return *this;
}

namespace {
template <typename TFrom>
FCesiumMetadataValue convertScalar(
    const TFrom& input,
    ECesiumMetadataComponentType targetComponentType) {
  switch (targetComponentType) {
  case ECesiumMetadataComponentType::Int8:
    return FCesiumMetadataValue(
        CesiumGltf::MetadataConversions<int8_t, TFrom>::convert(input));
  case ECesiumMetadataComponentType::Uint8:
    return FCesiumMetadataValue(
        CesiumGltf::MetadataConversions<uint8_t, TFrom>::convert(input));
  case ECesiumMetadataComponentType::Int16:
    return FCesiumMetadataValue(
        CesiumGltf::MetadataConversions<int16_t, TFrom>::convert(input));
  case ECesiumMetadataComponentType::Uint16:
    return FCesiumMetadataValue(
        CesiumGltf::MetadataConversions<uint16_t, TFrom>::convert(input));
  case ECesiumMetadataComponentType::Int32:
    return FCesiumMetadataValue(
        CesiumGltf::MetadataConversions<int32_t, TFrom>::convert(input));
  case ECesiumMetadataComponentType::Uint32:
    return FCesiumMetadataValue(
        CesiumGltf::MetadataConversions<uint32_t, TFrom>::convert(input));
  case ECesiumMetadataComponentType::Int64:
    return FCesiumMetadataValue(
        CesiumGltf::MetadataConversions<int64_t, TFrom>::convert(input));
  case ECesiumMetadataComponentType::Uint64:
    return FCesiumMetadataValue(
        CesiumGltf::MetadataConversions<uint64_t, TFrom>::convert(input));
  case ECesiumMetadataComponentType::Float32:
    return FCesiumMetadataValue(
        CesiumGltf::MetadataConversions<float, TFrom>::convert(input));
  case ECesiumMetadataComponentType::Float64:
    return FCesiumMetadataValue(
        CesiumGltf::MetadataConversions<double, TFrom>::convert(input));
  default:
    return EmptyMetadataValue;
  }
}

template <typename T>
std::vector<T> getScalarArray(const CesiumUtility::JsonValue::Array& array) {
  std::vector<T> result(array.size());

  for (int64 i = 0; i < int64(result.size()); i++) {
    std::optional<T> maybeConverted = std::nullopt;
    if (array[i].isInt64()) {
      maybeConverted = CesiumGltf::MetadataConversions<T, int64_t>::convert(
          array[i].getInt64());
    } else if (array[i].isUint64()) {
      maybeConverted = CesiumGltf::MetadataConversions<T, uint64_t>::convert(
          array[i].getUint64());
    } else if (array[i].isDouble()) {
      maybeConverted = CesiumGltf::MetadataConversions<T, double>::convert(
          array[i].getDouble());
    }

    if (!maybeConverted) {
      // This value is not a number or could not be converted; exit early.
      return std::vector<T>();
    }

    result[i] = *maybeConverted;
  }

  return result;
}

FCesiumMetadataValue convertToScalarArray(
    const CesiumUtility::JsonValue::Array& array,
    ECesiumMetadataComponentType targetComponentType) {
  FCesiumPropertyArray resultArray;
  switch (targetComponentType) {
  case ECesiumMetadataComponentType::Int8:
    resultArray = FCesiumPropertyArray(
        CesiumGltf::PropertyArrayCopy<int8_t>(getScalarArray<int8_t>(array)));
    break;
  case ECesiumMetadataComponentType::Uint8:
    resultArray = FCesiumPropertyArray(
        CesiumGltf::PropertyArrayCopy<uint8_t>(getScalarArray<uint8_t>(array)));
    break;
  case ECesiumMetadataComponentType::Int16:
    resultArray = FCesiumPropertyArray(
        CesiumGltf::PropertyArrayCopy<int16_t>(getScalarArray<int16_t>(array)));
    break;
  case ECesiumMetadataComponentType::Uint16:
    resultArray = FCesiumPropertyArray(CesiumGltf::PropertyArrayCopy<uint16_t>(
        getScalarArray<uint16_t>(array)));
    break;
  case ECesiumMetadataComponentType::Int32:
    resultArray = FCesiumPropertyArray(
        CesiumGltf::PropertyArrayCopy<int32_t>(getScalarArray<int32_t>(array)));
    break;
  case ECesiumMetadataComponentType::Uint32:
    resultArray = FCesiumPropertyArray(CesiumGltf::PropertyArrayCopy<uint32_t>(
        getScalarArray<uint32_t>(array)));
    break;
  case ECesiumMetadataComponentType::Int64:
    resultArray = FCesiumPropertyArray(
        CesiumGltf::PropertyArrayCopy<int64_t>(getScalarArray<int64_t>(array)));
    break;
  case ECesiumMetadataComponentType::Uint64:
    resultArray = FCesiumPropertyArray(CesiumGltf::PropertyArrayCopy<uint64_t>(
        getScalarArray<uint64_t>(array)));
    break;
  case ECesiumMetadataComponentType::Float32:
    resultArray = FCesiumPropertyArray(
        CesiumGltf::PropertyArrayCopy<float>(getScalarArray<float>(array)));
    break;
  case ECesiumMetadataComponentType::Float64:
    resultArray = FCesiumPropertyArray(
        CesiumGltf::PropertyArrayCopy<double>(getScalarArray<double>(array)));
    break;
  default:
    return EmptyMetadataValue;
  }

  return UCesiumPropertyArrayBlueprintLibrary::GetArraySize(resultArray)
             ? FCesiumMetadataValue(std::move(resultArray))
             : EmptyMetadataValue;
}

template <glm::length_t N, typename T>
std::optional<glm::vec<N, T>>
convertToVecN(const CesiumUtility::JsonValue::Array& array) {
  std::vector<T> components = getScalarArray<T>(array);
  if (components.size() != N) {
    return std::nullopt;
  }

  auto* pResult = reinterpret_cast<glm::vec<N, T>*>(components.data());
  return *pResult;
}

template <glm::length_t N>
FCesiumMetadataValue convertToVecN(
    const CesiumUtility::JsonValue::Array& array,
    ECesiumMetadataComponentType targetComponentType) {
  switch (targetComponentType) {
  case ECesiumMetadataComponentType::Int8:
    return FCesiumMetadataValue(convertToVecN<N, int8_t>(array));
  case ECesiumMetadataComponentType::Uint8:
    return FCesiumMetadataValue(convertToVecN<N, uint8_t>(array));
  case ECesiumMetadataComponentType::Int16:
    return FCesiumMetadataValue(convertToVecN<N, int16_t>(array));
  case ECesiumMetadataComponentType::Uint16:
    return FCesiumMetadataValue(convertToVecN<N, uint16_t>(array));
  case ECesiumMetadataComponentType::Int32:
    return FCesiumMetadataValue(convertToVecN<N, int32_t>(array));
  case ECesiumMetadataComponentType::Uint32:
    return FCesiumMetadataValue(convertToVecN<N, uint32_t>(array));
  case ECesiumMetadataComponentType::Int64:
    return FCesiumMetadataValue(convertToVecN<N, int64_t>(array));
  case ECesiumMetadataComponentType::Uint64:
    return FCesiumMetadataValue(convertToVecN<N, uint64_t>(array));
  case ECesiumMetadataComponentType::Float32:
    return FCesiumMetadataValue(convertToVecN<N, float>(array));
  case ECesiumMetadataComponentType::Float64:
    return FCesiumMetadataValue(convertToVecN<N, double>(array));
  default:
    return EmptyMetadataValue;
  }
}

template <glm::length_t N, typename T>
std::optional<glm::mat<N, N, T>>
convertToMatN(const CesiumUtility::JsonValue::Array& array) {
  std::vector<T> components = getScalarArray<T>(array);
  if (components.size() != N) {
    return std::nullopt;
  }

  auto pResult = reinterpret_cast<glm::mat<N, N, T>*>(components.data());
  return *pResult;
}

template <glm::length_t N>
FCesiumMetadataValue convertToMatN(
    const CesiumUtility::JsonValue::Array& array,
    ECesiumMetadataComponentType targetComponentType) {
  switch (targetComponentType) {
  case ECesiumMetadataComponentType::Int8:
    return FCesiumMetadataValue(convertToMatN<N, int8_t>(array));
  case ECesiumMetadataComponentType::Uint8:
    return FCesiumMetadataValue(convertToMatN<N, uint8_t>(array));
  case ECesiumMetadataComponentType::Int16:
    return FCesiumMetadataValue(convertToMatN<N, int16_t>(array));
  case ECesiumMetadataComponentType::Uint16:
    return FCesiumMetadataValue(convertToMatN<N, uint16_t>(array));
  case ECesiumMetadataComponentType::Int32:
    return FCesiumMetadataValue(convertToMatN<N, int32_t>(array));
  case ECesiumMetadataComponentType::Uint32:
    return FCesiumMetadataValue(convertToMatN<N, uint32_t>(array));
  case ECesiumMetadataComponentType::Int64:
    return FCesiumMetadataValue(convertToMatN<N, int64_t>(array));
  case ECesiumMetadataComponentType::Uint64:
    return FCesiumMetadataValue(convertToMatN<N, uint64_t>(array));
  case ECesiumMetadataComponentType::Float32:
    return FCesiumMetadataValue(convertToMatN<N, float>(array));
  case ECesiumMetadataComponentType::Float64:
    return FCesiumMetadataValue(convertToMatN<N, double>(array));
  default:
    return EmptyMetadataValue;
  }
}

template <glm::length_t N, typename T>
FCesiumMetadataValue
convertToVecNArray(const CesiumUtility::JsonValue::Array& array) {
  std::vector<glm::vec<N, T>> values(array.size());

  for (size_t i = 0; i < array.size(); i++) {
    if (!array[i].isArray() || array[i].getArray().size() != N) {
      return EmptyMetadataValue;
    }

    const CesiumUtility::JsonValue::Array& subArray = array[i].getArray();
    std::optional<glm::vec<N, T>> maybeValue = convertToVecN<N, T>(subArray);
    if (!maybeValue) {
      return EmptyMetadataValue;
    }
    values[i] = *maybeValue;
  }

  return FCesiumMetadataValue(
      CesiumGltf::PropertyArrayCopy<glm::vec<N, T>>(values));
}

template <glm::length_t N>
FCesiumMetadataValue convertToVecNArray(
    const CesiumUtility::JsonValue::Array& array,
    ECesiumMetadataComponentType targetComponentType) {
  switch (targetComponentType) {
  case ECesiumMetadataComponentType::Int8:
    return FCesiumMetadataValue(convertToVecNArray<N, int8_t>(array));
  case ECesiumMetadataComponentType::Uint8:
    return FCesiumMetadataValue(convertToVecNArray<N, uint8_t>(array));
  case ECesiumMetadataComponentType::Int16:
    return FCesiumMetadataValue(convertToVecNArray<N, int16_t>(array));
  case ECesiumMetadataComponentType::Uint16:
    return FCesiumMetadataValue(convertToVecNArray<N, uint16_t>(array));
  case ECesiumMetadataComponentType::Int32:
    return FCesiumMetadataValue(convertToVecNArray<N, int32_t>(array));
  case ECesiumMetadataComponentType::Uint32:
    return FCesiumMetadataValue(convertToVecNArray<N, uint32_t>(array));
  case ECesiumMetadataComponentType::Int64:
    return FCesiumMetadataValue(convertToVecNArray<N, int64_t>(array));
  case ECesiumMetadataComponentType::Uint64:
    return FCesiumMetadataValue(convertToVecNArray<N, uint64_t>(array));
  case ECesiumMetadataComponentType::Float32:
    return FCesiumMetadataValue(convertToVecNArray<N, float>(array));
  case ECesiumMetadataComponentType::Float64:
    return FCesiumMetadataValue(convertToVecNArray<N, double>(array));
  default:
    return EmptyMetadataValue;
  }
}

template <glm::length_t N, typename T>
FCesiumMetadataValue
convertToMatNArray(const CesiumUtility::JsonValue::Array& array) {
  std::vector<glm::mat<N, N, T>> values(array.size());

  for (size_t i = 0; i < array.size(); i++) {
    if (!array[i].isArray() || array[i].getArray().size() != N * N) {
      return EmptyMetadataValue;
    }

    const CesiumUtility::JsonValue::Array& subArray = array[i].getArray();
    std::optional<glm::mat<N, N, T>> maybeValue = convertToMatN<N, T>(subArray);
    if (!maybeValue) {
      return EmptyMetadataValue;
    }
    values[i] = *maybeValue;
  }

  return FCesiumMetadataValue(
      CesiumGltf::PropertyArrayCopy<glm::mat<N, N, T>>(values));
}

template <glm::length_t N>
FCesiumMetadataValue convertToMatNArray(
    const CesiumUtility::JsonValue::Array& array,
    ECesiumMetadataComponentType targetComponentType) {
  switch (targetComponentType) {
  case ECesiumMetadataComponentType::Int8:
    return FCesiumMetadataValue(convertToMatNArray<N, int8_t>(array));
  case ECesiumMetadataComponentType::Uint8:
    return FCesiumMetadataValue(convertToMatNArray<N, uint8_t>(array));
  case ECesiumMetadataComponentType::Int16:
    return FCesiumMetadataValue(convertToMatNArray<N, int16_t>(array));
  case ECesiumMetadataComponentType::Uint16:
    return FCesiumMetadataValue(convertToMatNArray<N, uint16_t>(array));
  case ECesiumMetadataComponentType::Int32:
    return FCesiumMetadataValue(convertToMatNArray<N, int32_t>(array));
  case ECesiumMetadataComponentType::Uint32:
    return FCesiumMetadataValue(convertToMatNArray<N, uint32_t>(array));
  case ECesiumMetadataComponentType::Int64:
    return FCesiumMetadataValue(convertToMatNArray<N, int64_t>(array));
  case ECesiumMetadataComponentType::Uint64:
    return FCesiumMetadataValue(convertToMatNArray<N, uint64_t>(array));
  case ECesiumMetadataComponentType::Float32:
    return FCesiumMetadataValue(convertToMatNArray<N, float>(array));
  case ECesiumMetadataComponentType::Float64:
    return FCesiumMetadataValue(convertToMatNArray<N, double>(array));
  default:
    return EmptyMetadataValue;
  }
}

} // namespace

/*static*/ FCesiumMetadataValue FCesiumMetadataValue::fromJsonArray(
    const CesiumUtility::JsonValue::Array& array,
    const FCesiumMetadataValueType& targetType) {
  if (array.empty()) {
    return EmptyMetadataValue;
  }

  if (!targetType.bIsArray) {
    // If the target type is not actually an array, then it must be one of the
    // following.
    switch (targetType.Type) {
    case ECesiumMetadataType::Vec2:
      return convertToVecN<2>(array, targetType.ComponentType);
    case ECesiumMetadataType::Vec3:
      return convertToVecN<3>(array, targetType.ComponentType);
    case ECesiumMetadataType::Vec4:
      return convertToVecN<4>(array, targetType.ComponentType);
    case ECesiumMetadataType::Mat2:
      return convertToMatN<2>(array, targetType.ComponentType);
    case ECesiumMetadataType::Mat3:
      return convertToMatN<3>(array, targetType.ComponentType);
    case ECesiumMetadataType::Mat4:
      return convertToMatN<4>(array, targetType.ComponentType);
      break;
    default:
      return EmptyMetadataValue;
    }
  }

  if (targetType.Type == ECesiumMetadataType::Boolean) {
    std::vector<bool> values(array.size());
    for (size_t i = 0; i < array.size(); i++) {
      if (!array[i].isBool()) {
        return EmptyMetadataValue;
      }
      values[i] = array[i].getBool();
    }
    return FCesiumMetadataValue(CesiumGltf::PropertyArrayCopy<bool>(values));
  }

  if (targetType.Type == ECesiumMetadataType::String) {
    std::vector<std::string> values(array.size());
    for (size_t i = 0; i < array.size(); i++) {
      if (!array[i].isString()) {
        return EmptyMetadataValue;
      }
      values[i] = array[i].getString();
    }
    return FCesiumMetadataValue(
        CesiumGltf::PropertyArrayCopy<std::string_view>(values));
  }

  if (targetType.Type == ECesiumMetadataType::Scalar ||
      targetType.Type == ECesiumMetadataType::Enum) {
    return convertToScalarArray(array, targetType.ComponentType);
  }

  // Nested arrays can indicate arrays of vecNs / matNs, and numeric data can be
  // handled in contiguous buffers.
  switch (targetType.Type) {
  case ECesiumMetadataType::Vec2:
    return convertToVecNArray<2>(array, targetType.ComponentType);
  case ECesiumMetadataType::Vec3:
    return convertToVecNArray<3>(array, targetType.ComponentType);
  case ECesiumMetadataType::Vec4:
    return convertToVecNArray<4>(array, targetType.ComponentType);
  case ECesiumMetadataType::Mat2:
    return convertToMatNArray<2>(array, targetType.ComponentType);
  case ECesiumMetadataType::Mat3:
    return convertToMatNArray<3>(array, targetType.ComponentType);
  case ECesiumMetadataType::Mat4:
    return convertToMatNArray<4>(array, targetType.ComponentType);
  default:
    return EmptyMetadataValue;
  }
}
/*static*/ FCesiumMetadataValue FCesiumMetadataValue::fromJsonValue(
    const CesiumUtility::JsonValue& jsonValue,
    const FCesiumMetadataValueType& targetType) {
  if (jsonValue.isArray()) {
    return fromJsonArray(jsonValue.getArray(), targetType);
  } else if (targetType.bIsArray) {
    return EmptyMetadataValue;
  }

  if (jsonValue.isBool()) {
    return targetType.Type == ECesiumMetadataType::Boolean
               ? FCesiumMetadataValue(jsonValue.getBool())
               : EmptyMetadataValue;
  }

  if (jsonValue.isString()) {
    return targetType.Type == ECesiumMetadataType::String
               ? FCesiumMetadataValue(std::string_view(jsonValue.getString()))
               : EmptyMetadataValue;
  }

  if (targetType.Type == ECesiumMetadataType::Scalar ||
      targetType.Type == ECesiumMetadataType::Enum) {
    if (jsonValue.isInt64()) {
      return FCesiumMetadataValue(
          convertScalar(jsonValue.getInt64(), targetType.ComponentType));
    }

    if (jsonValue.isUint64()) {
      return FCesiumMetadataValue(
          convertScalar(jsonValue.getUint64(), targetType.ComponentType));
    }

    if (jsonValue.isDouble()) {
      return FCesiumMetadataValue(
          convertScalar(jsonValue.getDouble(), targetType.ComponentType));
    }
  }

  return EmptyMetadataValue;
}

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

PRAGMA_ENABLE_DEPRECATION_WARNINGS

bool UCesiumMetadataValueBlueprintLibrary::GetBoolean(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    bool DefaultValue) {
  return swl::visit(
      [DefaultValue](auto value) -> bool {
        return CesiumGltf::MetadataConversions<bool, decltype(value)>::convert(
                   value)
            .value_or(DefaultValue);
      },
      Value._value);
}

uint8 UCesiumMetadataValueBlueprintLibrary::GetByte(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    uint8 DefaultValue) {
  return swl::visit(
      [DefaultValue](auto value) -> uint8 {
        return CesiumGltf::MetadataConversions<uint8, decltype(value)>::convert(
                   value)
            .value_or(DefaultValue);
      },
      Value._value);
}

int32 UCesiumMetadataValueBlueprintLibrary::GetInteger(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    int32 DefaultValue) {
  return swl::visit(
      [DefaultValue](auto value) {
        return CesiumGltf::MetadataConversions<int32, decltype(value)>::convert(
                   value)
            .value_or(DefaultValue);
      },
      Value._value);
}

int64 UCesiumMetadataValueBlueprintLibrary::GetInteger64(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    int64 DefaultValue) {
  return swl::visit(
      [DefaultValue](auto value) -> int64 {
        return CesiumGltf::MetadataConversions<int64_t, decltype(value)>::
            convert(value)
                .value_or(DefaultValue);
      },
      Value._value);
}

float UCesiumMetadataValueBlueprintLibrary::GetFloat(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    float DefaultValue) {
  return swl::visit(
      [DefaultValue](auto value) -> float {
        return CesiumGltf::MetadataConversions<float, decltype(value)>::convert(
                   value)
            .value_or(DefaultValue);
      },
      Value._value);
}

double UCesiumMetadataValueBlueprintLibrary::GetFloat64(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    double DefaultValue) {
  return swl::visit(
      [DefaultValue](auto value) -> double {
        return CesiumGltf::MetadataConversions<double, decltype(value)>::
            convert(value)
                .value_or(DefaultValue);
      },
      Value._value);
}

FIntPoint UCesiumMetadataValueBlueprintLibrary::GetIntPoint(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    const FIntPoint& DefaultValue) {
  return swl::visit(
      [&DefaultValue](auto value) -> FIntPoint {
        if constexpr (CesiumGltf::IsMetadataString<decltype(value)>::value) {
          return UnrealMetadataConversions::toIntPoint(value, DefaultValue);
        } else {
          auto maybeVec2 = CesiumGltf::
              MetadataConversions<glm::ivec2, decltype(value)>::convert(value);
          return maybeVec2 ? UnrealMetadataConversions::toIntPoint(*maybeVec2)
                           : DefaultValue;
        }
      },
      Value._value);
}

FVector2D UCesiumMetadataValueBlueprintLibrary::GetVector2D(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    const FVector2D& DefaultValue) {
  return swl::visit(
      [&DefaultValue](auto value) -> FVector2D {
        if constexpr (CesiumGltf::IsMetadataString<decltype(value)>::value) {
          return UnrealMetadataConversions::toVector2D(value, DefaultValue);
        } else {
          auto maybeVec2 = CesiumGltf::
              MetadataConversions<glm::dvec2, decltype(value)>::convert(value);
          return maybeVec2 ? UnrealMetadataConversions::toVector2D(*maybeVec2)
                           : DefaultValue;
        }
      },
      Value._value);
}

FIntVector UCesiumMetadataValueBlueprintLibrary::GetIntVector(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    const FIntVector& DefaultValue) {
  return swl::visit(
      [&DefaultValue](auto value) -> FIntVector {
        if constexpr (CesiumGltf::IsMetadataString<decltype(value)>::value) {
          return UnrealMetadataConversions::toIntVector(value, DefaultValue);
        } else {
          auto maybeVec3 = CesiumGltf::
              MetadataConversions<glm::ivec3, decltype(value)>::convert(value);
          return maybeVec3 ? UnrealMetadataConversions::toIntVector(*maybeVec3)
                           : DefaultValue;
        }
      },
      Value._value);
}

FVector3f UCesiumMetadataValueBlueprintLibrary::GetVector3f(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    const FVector3f& DefaultValue) {
  return swl::visit(
      [&DefaultValue](auto value) -> FVector3f {
        if constexpr (CesiumGltf::IsMetadataString<decltype(value)>::value) {
          return UnrealMetadataConversions::toVector3f(value, DefaultValue);
        } else {
          auto maybeVec3 = CesiumGltf::
              MetadataConversions<glm::vec3, decltype(value)>::convert(value);
          return maybeVec3 ? UnrealMetadataConversions::toVector3f(*maybeVec3)
                           : DefaultValue;
        }
      },
      Value._value);
}

FVector UCesiumMetadataValueBlueprintLibrary::GetVector(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    const FVector& DefaultValue) {
  return swl::visit(
      [&DefaultValue](auto value) -> FVector {
        if constexpr (CesiumGltf::IsMetadataString<decltype(value)>::value) {
          return UnrealMetadataConversions::toVector(value, DefaultValue);
        } else {
          auto maybeVec3 = CesiumGltf::
              MetadataConversions<glm::dvec3, decltype(value)>::convert(value);
          return maybeVec3 ? UnrealMetadataConversions::toVector(*maybeVec3)
                           : DefaultValue;
        }
      },
      Value._value);
}

FVector4 UCesiumMetadataValueBlueprintLibrary::GetVector4(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    const FVector4& DefaultValue) {
  return swl::visit(
      [&DefaultValue](auto value) -> FVector4 {
        if constexpr (CesiumGltf::IsMetadataString<decltype(value)>::value) {
          return UnrealMetadataConversions::toVector4(value, DefaultValue);
        } else {
          auto maybeVec4 = CesiumGltf::
              MetadataConversions<glm::dvec4, decltype(value)>::convert(value);
          return maybeVec4 ? UnrealMetadataConversions::toVector4(*maybeVec4)
                           : DefaultValue;
        }
      },
      Value._value);
}

FMatrix UCesiumMetadataValueBlueprintLibrary::GetMatrix(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    const FMatrix& DefaultValue) {
  auto maybeMat4 = swl::visit(
      [&DefaultValue](auto value) -> std::optional<glm::dmat4> {
        return CesiumGltf::MetadataConversions<glm::dmat4, decltype(value)>::
            convert(value);
      },
      Value._value);

  return maybeMat4 ? UnrealMetadataConversions::toMatrix(*maybeMat4)
                   : DefaultValue;
}

FString UCesiumMetadataValueBlueprintLibrary::GetString(
    UPARAM(ref) const FCesiumMetadataValue& Value,
    const FString& DefaultValue) {
  if (Value._arrayValue) {
    return UCesiumPropertyArrayBlueprintLibrary::ToString(
        *Value._arrayValue,
        DefaultValue);
  }

  return swl::visit(
      [&DefaultValue, &Value](auto value) -> FString {
        using ValueType = decltype(value);
        if constexpr (
            CesiumGltf::IsMetadataVecN<ValueType>::value ||
            CesiumGltf::IsMetadataMatN<ValueType>::value ||
            CesiumGltf::IsMetadataString<ValueType>::value) {
          return UnrealMetadataConversions::toString(value);
        } else {
          if constexpr (CesiumGltf::IsMetadataInteger<ValueType>::value) {
            if (Value._pEnumDefinition.IsValid()) {
              return Value._pEnumDefinition->GetName(value).Get(DefaultValue);
            }
          }

          auto maybeString = CesiumGltf::
              MetadataConversions<std::string, decltype(value)>::convert(value);

          return maybeString ? UnrealMetadataConversions::toString(*maybeString)
                             : DefaultValue;
        }
      },
      Value._value);
}

FCesiumPropertyArray UCesiumMetadataValueBlueprintLibrary::GetArray(
    UPARAM(ref) const FCesiumMetadataValue& Value) {
  return Value._arrayValue.Get(FCesiumPropertyArray());
}

bool UCesiumMetadataValueBlueprintLibrary::IsEmpty(
    UPARAM(ref) const FCesiumMetadataValue& Value) {
  return swl::holds_alternative<swl::monostate>(Value._value) &&
         !Value._arrayValue.IsSet();
}

TMap<FString, FString> UCesiumMetadataValueBlueprintLibrary::GetValuesAsStrings(
    const TMap<FString, FCesiumMetadataValue>& Values) {
  TMap<FString, FString> strings;
  for (auto valuesIt : Values) {
    strings.Add(
        valuesIt.Key,
        UCesiumMetadataValueBlueprintLibrary::GetString(
            valuesIt.Value,
            FString()));
  }

  return strings;
}

uint64 CesiumMetadataValueAccess::GetUnsignedInteger64(
    const FCesiumMetadataValue& Value,
    uint64 DefaultValue) {
  return swl::visit(
      [DefaultValue](auto value) -> uint64 {
        return CesiumGltf::MetadataConversions<uint64_t, decltype(value)>::
            convert(value)
                .value_or(DefaultValue);
      },
      Value._value);
}
