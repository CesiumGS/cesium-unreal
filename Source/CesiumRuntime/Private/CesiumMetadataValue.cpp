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
  FCesiumMetadataValueType elementType = Array._elementType;
  this->_valueType = {elementType.Type, elementType.ComponentType, true};
  this->_arrayValue = std::move(Array);
}

FCesiumMetadataValue::FCesiumMetadataValue(FCesiumMetadataValue&& rhs) =
    default;

FCesiumMetadataValue&
FCesiumMetadataValue::operator=(FCesiumMetadataValue&& rhs) = default;

FCesiumMetadataValue::FCesiumMetadataValue(const FCesiumMetadataValue& rhs)
    : _value(rhs._value),
      _arrayValue(rhs._arrayValue),
      _valueType(rhs._valueType),
      _pEnumDefinition(rhs._pEnumDefinition) {}

FCesiumMetadataValue&
FCesiumMetadataValue::operator=(const FCesiumMetadataValue& rhs) {
  *this = FCesiumMetadataValue(rhs);
  return *this;
}

namespace {
template <typename TTo>
std::optional<TTo> convertScalar(const CesiumUtility::JsonValue& value) {
  if (value.isInt64()) {
    return CesiumGltf::MetadataConversions<TTo, int64_t>::convert(
        value.getInt64());
  }

  if (value.isUint64()) {
    return CesiumGltf::MetadataConversions<TTo, uint64_t>::convert(
        value.getUint64());
  }

  if (value.isDouble()) {
    return CesiumGltf::MetadataConversions<TTo, double>::convert(
        value.getDouble());
  }

  return std::nullopt;
}

template <typename T>
std::vector<T> getScalarArray(const CesiumUtility::JsonValue::Array& array) {
  std::vector<T> result(array.size());

  for (int64 i = 0; i < int64(result.size()); i++) {
    std::optional<T> maybeConverted = convertScalar<T>(array[i]);
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

template <typename T>
FCesiumMetadataValue
convertToVecOrMat(const std::vector<T>& components, ECesiumMetadataType type) {
  switch (type) {
  case ECesiumMetadataType::Vec2:
    return components.size() == 2
               ? FCesiumMetadataValue(*reinterpret_cast<const glm::vec<2, T>*>(
                     components.data()))
               : EmptyMetadataValue;
  case ECesiumMetadataType::Vec3:
    return components.size() == 3
               ? FCesiumMetadataValue(*reinterpret_cast<const glm::vec<3, T>*>(
                     components.data()))
               : EmptyMetadataValue;
  case ECesiumMetadataType::Vec4:
    return components.size() == 4
               ? FCesiumMetadataValue(*reinterpret_cast<const glm::vec<4, T>*>(
                     components.data()))
               : EmptyMetadataValue;
  case ECesiumMetadataType::Mat2:
    return components.size() == 4
               ? FCesiumMetadataValue(
                     *reinterpret_cast<const glm::mat<2, 2, T>*>(
                         components.data()))
               : EmptyMetadataValue;
  case ECesiumMetadataType::Mat3:
    return components.size() == 9
               ? FCesiumMetadataValue(
                     *reinterpret_cast<const glm::mat<3, 3, T>*>(
                         components.data()))
               : EmptyMetadataValue;
  case ECesiumMetadataType::Mat4:
    return components.size() == 16
               ? FCesiumMetadataValue(
                     *reinterpret_cast<const glm::mat<4, 4, T>*>(
                         components.data()))
               : EmptyMetadataValue;
  default:
    return EmptyMetadataValue;
  }
}

FCesiumMetadataValue convertToVecOrMat(
    const CesiumUtility::JsonValue::Array& array,
    const FCesiumMetadataValueType& targetType) {
  switch (targetType.Type) {
  case ECesiumMetadataType::Vec2:
  case ECesiumMetadataType::Vec3:
  case ECesiumMetadataType::Vec4:
  case ECesiumMetadataType::Mat2:
  case ECesiumMetadataType::Mat3:
  case ECesiumMetadataType::Mat4:
    break;
  default:
    return EmptyMetadataValue;
  }

  switch (targetType.ComponentType) {
  case ECesiumMetadataComponentType::Int8: {
    std::vector<int8_t> result = getScalarArray<int8_t>(array);
    return result.size() ? convertToVecOrMat(result, targetType.Type)
                         : EmptyMetadataValue;
  }
  case ECesiumMetadataComponentType::Uint8: {
    std::vector<uint8_t> result = getScalarArray<uint8_t>(array);
    return result.size() ? convertToVecOrMat(result, targetType.Type)
                         : EmptyMetadataValue;
  }
  case ECesiumMetadataComponentType::Int16: {
    std::vector<int16_t> result = getScalarArray<int16_t>(array);
    return result.size() ? convertToVecOrMat(result, targetType.Type)
                         : EmptyMetadataValue;
  }
  case ECesiumMetadataComponentType::Uint16: {
    std::vector<uint16_t> result = getScalarArray<uint16_t>(array);
    return result.size() ? convertToVecOrMat(result, targetType.Type)
                         : EmptyMetadataValue;
  }
  case ECesiumMetadataComponentType::Int32: {
    std::vector<int32_t> result = getScalarArray<int32_t>(array);
    return result.size() ? convertToVecOrMat(result, targetType.Type)
                         : EmptyMetadataValue;
  }
  case ECesiumMetadataComponentType::Uint32: {
    std::vector<uint32_t> result = getScalarArray<uint32_t>(array);
    return result.size() ? convertToVecOrMat(result, targetType.Type)
                         : EmptyMetadataValue;
  }
  case ECesiumMetadataComponentType::Int64: {
    std::vector<int64_t> result = getScalarArray<int64_t>(array);
    return result.size() ? convertToVecOrMat(result, targetType.Type)
                         : EmptyMetadataValue;
  }
  case ECesiumMetadataComponentType::Uint64: {
    std::vector<uint64_t> result = getScalarArray<uint64_t>(array);
    return result.size() ? convertToVecOrMat(result, targetType.Type)
                         : EmptyMetadataValue;
  }
  case ECesiumMetadataComponentType::Float32: {
    std::vector<float> result = getScalarArray<float>(array);
    return result.size() ? convertToVecOrMat(result, targetType.Type)
                         : EmptyMetadataValue;
  }
  case ECesiumMetadataComponentType::Float64: {
    std::vector<double> result = getScalarArray<double>(array);
    return result.size() ? convertToVecOrMat(result, targetType.Type)
                         : EmptyMetadataValue;
  }
  default:
    return EmptyMetadataValue;
  }
}

template <typename T>
FCesiumMetadataValue
convertToVecOrMatArray(std::vector<T>&& components, ECesiumMetadataType type) {
  if (components.empty())
    return EmptyMetadataValue;

  switch (type) {
  case ECesiumMetadataType::Vec2: {
    std::vector<glm::vec<2, T>> reinterpreted(components.size() / 2);
    std::memcpy(
        reinterpreted.data(),
        components.data(),
        components.size() * sizeof(T));

    return FCesiumMetadataValue(CesiumGltf::PropertyArrayCopy<glm::vec<2, T>>(
        std::move(reinterpreted)));
  }
  case ECesiumMetadataType::Vec3: {
    std::vector<glm::vec<3, T>> reinterpreted(components.size() / 3);
    std::memcpy(
        reinterpreted.data(),
        components.data(),
        components.size() * sizeof(T));

    return FCesiumMetadataValue(CesiumGltf::PropertyArrayCopy<glm::vec<3, T>>(
        std::move(reinterpreted)));
  }
  case ECesiumMetadataType::Vec4: {
    std::vector<glm::vec<4, T>> reinterpreted(components.size() / 4);
    std::memcpy(
        reinterpreted.data(),
        components.data(),
        components.size() * sizeof(T));

    return FCesiumMetadataValue(CesiumGltf::PropertyArrayCopy<glm::vec<4, T>>(
        std::move(reinterpreted)));
  }
  case ECesiumMetadataType::Mat2: {
    std::vector<glm::mat<2, 2, T>> reinterpreted(components.size() / 4);
    std::memcpy(
        reinterpreted.data(),
        components.data(),
        components.size() * sizeof(T));

    return FCesiumMetadataValue(
        CesiumGltf::PropertyArrayCopy<glm::mat<2, 2, T>>(
            std::move(reinterpreted)));
  }
  case ECesiumMetadataType::Mat3: {
    std::vector<glm::mat<3, 3, T>> reinterpreted(components.size() / 9);
    std::memcpy(
        reinterpreted.data(),
        components.data(),
        components.size() * sizeof(T));

    return FCesiumMetadataValue(
        CesiumGltf::PropertyArrayCopy<glm::mat<3, 3, T>>(
            std::move(reinterpreted)));
  }
  case ECesiumMetadataType::Mat4: {
    std::vector<glm::mat<4, 4, T>> reinterpreted(components.size() / 16);
    std::memcpy(
        reinterpreted.data(),
        components.data(),
        components.size() * sizeof(T));

    return FCesiumMetadataValue(
        CesiumGltf::PropertyArrayCopy<glm::mat<4, 4, T>>(
            std::move(reinterpreted)));
  }
  default:
    return EmptyMetadataValue;
  }
}

FCesiumMetadataValue convertToVecOrMatArray(
    const CesiumUtility::JsonValue::Array& array,
    const FCesiumMetadataValueType& targetType) {
  size_t expectedElementSize = 0;
  switch (targetType.Type) {
  case ECesiumMetadataType::Vec2:
    expectedElementSize = 2;
    break;
  case ECesiumMetadataType::Vec3:
    expectedElementSize = 3;
    break;
  case ECesiumMetadataType::Vec4:
  case ECesiumMetadataType::Mat2:
    expectedElementSize = 4;
    break;
  case ECesiumMetadataType::Mat3:
    expectedElementSize = 9;
    break;
  case ECesiumMetadataType::Mat4:
    expectedElementSize = 16;
    break;
  default:
    return EmptyMetadataValue;
  }

  // Although this is code could be templatized, it is intentionally not so to
  // preserve compiler times.
  switch (targetType.ComponentType) {
  case ECesiumMetadataComponentType::Int8: {
    std::vector<int8_t> result;
    for (size_t i = 0; i < array.size(); i++) {
      if (!array[i].isArray())
        return EmptyMetadataValue;

      const CesiumUtility::JsonValue::Array& subArray = array[i].getArray();
      std::vector<int8_t> convertedArray = getScalarArray<int8_t>(subArray);
      if (convertedArray.size() != expectedElementSize)
        return EmptyMetadataValue;

      result.insert(result.end(), convertedArray.begin(), convertedArray.end());
    }
    return convertToVecOrMatArray(std::move(result), targetType.Type);
  }
  case ECesiumMetadataComponentType::Uint8: {
    std::vector<uint8_t> result;
    for (size_t i = 0; i < array.size(); i++) {
      if (!array[i].isArray())
        return EmptyMetadataValue;

      const CesiumUtility::JsonValue::Array& subArray = array[i].getArray();
      std::vector<uint8_t> convertedArray = getScalarArray<uint8_t>(subArray);
      if (convertedArray.size() != expectedElementSize)
        return EmptyMetadataValue;

      result.insert(result.end(), convertedArray.begin(), convertedArray.end());
    }
    return convertToVecOrMatArray(std::move(result), targetType.Type);
  }
  case ECesiumMetadataComponentType::Int16: {
    std::vector<int16_t> result;
    for (size_t i = 0; i < array.size(); i++) {
      if (!array[i].isArray())
        return EmptyMetadataValue;

      const CesiumUtility::JsonValue::Array& subArray = array[i].getArray();
      std::vector<int16_t> convertedArray = getScalarArray<int16_t>(subArray);
      if (convertedArray.size() != expectedElementSize)
        return EmptyMetadataValue;

      result.insert(result.end(), convertedArray.begin(), convertedArray.end());
    }
    return convertToVecOrMatArray(std::move(result), targetType.Type);
  }
  case ECesiumMetadataComponentType::Uint16: {
    std::vector<uint16_t> result;
    for (size_t i = 0; i < array.size(); i++) {
      if (!array[i].isArray())
        return EmptyMetadataValue;

      const CesiumUtility::JsonValue::Array& subArray = array[i].getArray();
      std::vector<uint16_t> convertedArray = getScalarArray<uint16_t>(subArray);
      if (convertedArray.size() != expectedElementSize)
        return EmptyMetadataValue;

      result.insert(result.end(), convertedArray.begin(), convertedArray.end());
    }
    return convertToVecOrMatArray(std::move(result), targetType.Type);
  }
  case ECesiumMetadataComponentType::Int32: {
    std::vector<int32_t> result;
    for (size_t i = 0; i < array.size(); i++) {
      if (!array[i].isArray())
        return EmptyMetadataValue;

      const CesiumUtility::JsonValue::Array& subArray = array[i].getArray();
      std::vector<int32_t> convertedArray = getScalarArray<int32_t>(subArray);
      if (convertedArray.size() != expectedElementSize)
        return EmptyMetadataValue;

      result.insert(result.end(), convertedArray.begin(), convertedArray.end());
    }
    return convertToVecOrMatArray(std::move(result), targetType.Type);
  }
  case ECesiumMetadataComponentType::Uint32: {
    std::vector<uint32_t> result;
    for (size_t i = 0; i < array.size(); i++) {
      if (!array[i].isArray())
        return EmptyMetadataValue;

      const CesiumUtility::JsonValue::Array& subArray = array[i].getArray();
      std::vector<uint32_t> convertedArray = getScalarArray<uint32_t>(subArray);
      if (convertedArray.size() != expectedElementSize)
        return EmptyMetadataValue;

      result.insert(result.end(), convertedArray.begin(), convertedArray.end());
    }
    return convertToVecOrMatArray(std::move(result), targetType.Type);
  }
  case ECesiumMetadataComponentType::Int64: {
    std::vector<int64_t> result;
    for (size_t i = 0; i < array.size(); i++) {
      if (!array[i].isArray())
        return EmptyMetadataValue;

      const CesiumUtility::JsonValue::Array& subArray = array[i].getArray();
      std::vector<int64_t> convertedArray = getScalarArray<int64_t>(subArray);
      if (convertedArray.size() != expectedElementSize)
        return EmptyMetadataValue;

      result.insert(result.end(), convertedArray.begin(), convertedArray.end());
    }
    return convertToVecOrMatArray(std::move(result), targetType.Type);
  }
  case ECesiumMetadataComponentType::Uint64: {
    std::vector<uint64_t> result;
    for (size_t i = 0; i < array.size(); i++) {
      if (!array[i].isArray())
        return EmptyMetadataValue;

      const CesiumUtility::JsonValue::Array& subArray = array[i].getArray();
      std::vector<uint64_t> convertedArray = getScalarArray<uint64_t>(subArray);
      if (convertedArray.size() != expectedElementSize)
        return EmptyMetadataValue;

      result.insert(result.end(), convertedArray.begin(), convertedArray.end());
    }
    return convertToVecOrMatArray(std::move(result), targetType.Type);
  }
  case ECesiumMetadataComponentType::Float32: {
    std::vector<float> result;
    for (size_t i = 0; i < array.size(); i++) {
      if (!array[i].isArray())
        return EmptyMetadataValue;

      const CesiumUtility::JsonValue::Array& subArray = array[i].getArray();
      std::vector<float> convertedArray = getScalarArray<float>(subArray);
      if (convertedArray.size() != expectedElementSize)
        return EmptyMetadataValue;

      result.insert(result.end(), convertedArray.begin(), convertedArray.end());
    }
    return convertToVecOrMatArray(std::move(result), targetType.Type);
  }
  case ECesiumMetadataComponentType::Float64: {
    std::vector<double> result;
    for (size_t i = 0; i < array.size(); i++) {
      if (!array[i].isArray())
        return EmptyMetadataValue;

      const CesiumUtility::JsonValue::Array& subArray = array[i].getArray();
      std::vector<double> convertedArray = getScalarArray<double>(subArray);
      if (convertedArray.size() != expectedElementSize)
        return EmptyMetadataValue;

      result.insert(result.end(), convertedArray.begin(), convertedArray.end());
    }
    return convertToVecOrMatArray(std::move(result), targetType.Type);
  }
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
    // If the target type is not actually an array, then it must be a vector or
    // matrix value.
    return convertToVecOrMat(array, targetType);
  }

  switch (targetType.Type) {
  case ECesiumMetadataType::Scalar:
  case ECesiumMetadataType::Enum:
    return convertToScalarArray(array, targetType.ComponentType);
  case ECesiumMetadataType::Vec2:
  case ECesiumMetadataType::Vec3:
  case ECesiumMetadataType::Vec4:
  case ECesiumMetadataType::Mat2:
  case ECesiumMetadataType::Mat3:
  case ECesiumMetadataType::Mat4:
    return convertToVecOrMatArray(array, targetType);
  case ECesiumMetadataType::Boolean: {
    std::vector<bool> values(array.size());
    for (size_t i = 0; i < array.size(); i++) {
      if (!array[i].isBool()) {
        return EmptyMetadataValue;
      }
      values[i] = array[i].getBool();
    }
    return FCesiumMetadataValue(CesiumGltf::PropertyArrayCopy<bool>(values));
  }
  case ECesiumMetadataType::String: {
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

  switch (targetType.Type) {
  case ECesiumMetadataType::Boolean:
    return jsonValue.isBool() ? FCesiumMetadataValue(jsonValue.getBool())
                              : EmptyMetadataValue;
  case ECesiumMetadataType::String:
    return jsonValue.isString()
               ? FCesiumMetadataValue(std::string_view(jsonValue.getString()))
               : EmptyMetadataValue;
  case ECesiumMetadataType::Scalar:
  case ECesiumMetadataType::Enum:
    break;
  default:
    return EmptyMetadataValue;
  }

  switch (targetType.ComponentType) {
  case ECesiumMetadataComponentType::Int8:
    return FCesiumMetadataValue(convertScalar<int8_t>(jsonValue));
  case ECesiumMetadataComponentType::Uint8:
    return FCesiumMetadataValue(convertScalar<uint8_t>(jsonValue));
  case ECesiumMetadataComponentType::Int16:
    return FCesiumMetadataValue(convertScalar<int16_t>(jsonValue));
  case ECesiumMetadataComponentType::Uint16:
    return FCesiumMetadataValue(convertScalar<uint16_t>(jsonValue));
  case ECesiumMetadataComponentType::Int32:
    return FCesiumMetadataValue(convertScalar<int32_t>(jsonValue));
  case ECesiumMetadataComponentType::Uint32:
    return FCesiumMetadataValue(convertScalar<uint32_t>(jsonValue));
  case ECesiumMetadataComponentType::Int64:
    return FCesiumMetadataValue(convertScalar<int64_t>(jsonValue));
  case ECesiumMetadataComponentType::Uint64:
    return FCesiumMetadataValue(convertScalar<uint64_t>(jsonValue));
  case ECesiumMetadataComponentType::Float32:
    return FCesiumMetadataValue(convertScalar<float>(jsonValue));
  case ECesiumMetadataComponentType::Float64:
    return FCesiumMetadataValue(convertScalar<double>(jsonValue));
  default:
    return EmptyMetadataValue;
  }
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
    return UCesiumPropertyArrayBlueprintLibrary::ToString(*Value._arrayValue);
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
              TOptional<FString> maybeName =
                  Value._pEnumDefinition->GetName(value);
              return maybeName.Get(DefaultValue);
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
