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
struct ComponentTypeFit {
  ECesiumMetadataComponentType smallestSignedType =
      ECesiumMetadataComponentType::None;
  ECesiumMetadataComponentType smallestUnsignedType =
      ECesiumMetadataComponentType::None;
  ECesiumMetadataComponentType smallestFloatingType =
      ECesiumMetadataComponentType::None;

  inline bool isValid() const {
    return smallestSignedType != ECesiumMetadataComponentType::None ||
           smallestUnsignedType != ECesiumMetadataComponentType::None ||
           smallestFloatingType != ECesiumMetadataComponentType::None;
  }

  bool
  isCompatibleWithComponentType(ECesiumMetadataComponentType componentType) {
    // If the smallest-fitting type is bigger than the desired one, the value
    // cannot be losslessly converted.
    switch (componentType) {
    case ECesiumMetadataComponentType::Float32:
    case ECesiumMetadataComponentType::Float64:
      return smallestFloatingType != ECesiumMetadataComponentType::None &&
             smallestFloatingType <= componentType;
    case ECesiumMetadataComponentType::Int8:
    case ECesiumMetadataComponentType::Int16:
    case ECesiumMetadataComponentType::Int32:
    case ECesiumMetadataComponentType::Int64:
      return smallestSignedType != ECesiumMetadataComponentType::None &&
             smallestSignedType <= componentType;
    case ECesiumMetadataComponentType::Uint8:
    case ECesiumMetadataComponentType::Uint16:
    case ECesiumMetadataComponentType::Uint32:
    case ECesiumMetadataComponentType::Uint64:
      return smallestUnsignedType != ECesiumMetadataComponentType::None &&
             smallestUnsignedType <= componentType;
    default:
      return false;
    }
  }

  void operator&=(const ComponentTypeFit& other) {
    if (smallestSignedType > ECesiumMetadataComponentType::None) {
      smallestSignedType =
          (other.smallestSignedType > ECesiumMetadataComponentType::None)
              ? std::max(smallestSignedType, other.smallestSignedType)
              : ECesiumMetadataComponentType::None;
    }

    if (smallestUnsignedType > ECesiumMetadataComponentType::None) {
      smallestUnsignedType =
          (other.smallestUnsignedType > ECesiumMetadataComponentType::None)
              ? std::max(smallestUnsignedType, other.smallestUnsignedType)
              : ECesiumMetadataComponentType::None;
    }

    if (smallestFloatingType > ECesiumMetadataComponentType::None) {
      smallestFloatingType =
          (other.smallestFloatingType > ECesiumMetadataComponentType::None)
              ? std::max(smallestFloatingType, other.smallestFloatingType)
              : ECesiumMetadataComponentType::None;
    }
  }
};

ComponentTypeFit
getBestComponentTypeFit(const CesiumUtility::JsonValue& value) {
  ComponentTypeFit result;

  std::optional<int64_t> valueAsInt;
  std::optional<uint64_t> valueAsUint;
  std::optional<double> valueAsDouble;

  if (value.isInt64()) {
    valueAsInt = value.getInt64();
    if (*valueAsInt >= 0) {
      valueAsUint = uint64_t(*valueAsInt);
    }
    if (CesiumUtility::losslessNarrow<double>(*valueAsInt).has_value()) {
      valueAsDouble = double(*valueAsInt);
    }
  } else if (value.isUint64()) {
    valueAsUint = value.getUint64();
    if (*valueAsUint <= uint64_t(std::numeric_limits<int64_t>::max())) {
      valueAsInt = int64_t(*valueAsUint);
    }
    if (CesiumUtility::losslessNarrow<double>(*valueAsUint).has_value()) {
      valueAsDouble = double(*valueAsUint);
    }
  } else if (value.isDouble()) {
    valueAsDouble = value.getDouble();
    if (CesiumUtility::losslessNarrow<int64_t>(*valueAsDouble).has_value()) {
      valueAsInt = int64_t(*valueAsDouble);
    }
    if (CesiumUtility::losslessNarrow<uint64_t>(*valueAsDouble).has_value()) {
      valueAsUint = uint64_t(*valueAsDouble);
    }
  }

  if (valueAsInt) {
    if (CesiumUtility::losslessNarrow<int8_t>(*valueAsInt).has_value())
      result.smallestSignedType = ECesiumMetadataComponentType::Int8;
    else if (CesiumUtility::losslessNarrow<int16_t>(*valueAsInt).has_value())
      result.smallestSignedType = ECesiumMetadataComponentType::Int16;
    else if (CesiumUtility::losslessNarrow<int32_t>(*valueAsInt).has_value())
      result.smallestSignedType = ECesiumMetadataComponentType::Int32;
    else
      result.smallestSignedType = ECesiumMetadataComponentType::Int64;

    if (CesiumUtility::losslessNarrow<float>(*valueAsInt).has_value())
      result.smallestFloatingType = ECesiumMetadataComponentType::Float32;
    else if (CesiumUtility::losslessNarrow<double>(*valueAsInt).has_value())
      result.smallestFloatingType = ECesiumMetadataComponentType::Float64;
  }

  if (valueAsUint) {
    if (*valueAsUint <= std::numeric_limits<uint8_t>::max())
      result.smallestUnsignedType = ECesiumMetadataComponentType::Uint8;
    else if (*valueAsUint <= std::numeric_limits<uint16_t>::max())
      result.smallestUnsignedType = ECesiumMetadataComponentType::Uint16;
    else if (*valueAsUint <= std::numeric_limits<uint32_t>::max())
      result.smallestUnsignedType = ECesiumMetadataComponentType::Uint32;
    else
      result.smallestUnsignedType = ECesiumMetadataComponentType::Uint64;

    if (CesiumUtility::losslessNarrow<float>(*valueAsUint).has_value())
      result.smallestFloatingType = ECesiumMetadataComponentType::Float32;
    else if (CesiumUtility::losslessNarrow<double>(*valueAsUint).has_value())
      result.smallestFloatingType = ECesiumMetadataComponentType::Float64;
  }

  if (valueAsDouble) {
    // Intentionally more lax with floating point conversions.
    std::optional<float> maybeFloat =
        CesiumGltf::MetadataConversions<float, double>::convert(*valueAsDouble);
    result.smallestFloatingType = maybeFloat
                                      ? ECesiumMetadataComponentType::Float32
                                      : ECesiumMetadataComponentType::Float64;
  }

  return result;
}

ComponentTypeFit
getBestComponentTypeFit(const CesiumUtility::JsonValue::Array& array) {
  if (array.empty())
    return ComponentTypeFit{};

  ComponentTypeFit result = getBestComponentTypeFit(array[0]);
  for (size_t i = 1; i < array.size(); i++) {
    result &= getBestComponentTypeFit(array[i]);
  }
  return result;
}

std::vector<int64_t>
getInt64Array(const CesiumUtility::JsonValue::Array& array) {
  std::vector<int64_t> result(array.size());
  for (int64 i = 0; i < int64(result.size()); i++) {
    if (array[i].isInt64()) {
      result[i] = array[i].getInt64();
    } else if (array[i].isUint64()) {
      result[i] = static_cast<int64_t>(array[i].getUint64());
    } else if (array[i].isDouble()) {
      result[i] = static_cast<int64_t>(array[i].getDouble());
    } else {
      return std::vector<int64_t>();
    }
  }
  return result;
}

std::vector<uint64_t>
getUint64Array(const CesiumUtility::JsonValue::Array& array) {
  std::vector<uint64_t> result(array.size());
  for (int64 i = 0; i < int64(result.size()); i++) {
    if (array[i].isInt64()) {
      result[i] = static_cast<uint64_t>(array[i].getInt64());
    } else if (array[i].isUint64()) {
      result[i] = array[i].getUint64();
    } else if (array[i].isDouble()) {
      result[i] = static_cast<uint64_t>(array[i].getDouble());
    } else {
      return std::vector<uint64_t>();
    }
  }
  return result;
}

std::vector<double>
getDoubleArray(const CesiumUtility::JsonValue::Array& array) {
  std::vector<double> result(array.size());
  for (int64 i = 0; i < int64(result.size()); i++) {
    if (array[i].isInt64()) {
      result[i] = static_cast<double>(array[i].getInt64());
    } else if (array[i].isUint64()) {
      result[i] = static_cast<double>(array[i].getUint64());
    } else if (array[i].isDouble()) {
      result[i] = array[i].getDouble();
    } else {
      return std::vector<double>();
    }
  }
  return result;
}

} // namespace

void FCesiumMetadataValue::initializeAsScalarArray(
    const CesiumUtility::JsonValue::Array& array,
    const FCesiumMetadataValueType& targetType) {
  ComponentTypeFit bestFit = getBestComponentTypeFit(array);
  if (!bestFit.isCompatibleWithComponentType(targetType.ComponentType))
    return;

  FCesiumPropertyArray resultArray;

  switch (targetType.ComponentType) {
  case ECesiumMetadataComponentType::Float32:
  case ECesiumMetadataComponentType::Float64: {
    std::vector<double> values = getDoubleArray(array);
    resultArray = FCesiumPropertyArray(
        CesiumGltf::PropertyArrayCopy<double>(std::move(values)));
    break;
  }
  case ECesiumMetadataComponentType::Int8:
  case ECesiumMetadataComponentType::Int16:
  case ECesiumMetadataComponentType::Int32:
  case ECesiumMetadataComponentType::Int64: {
    std::vector<int64_t> values = getInt64Array(array);
    resultArray = FCesiumPropertyArray(
        CesiumGltf::PropertyArrayCopy<int64_t>(std::move(values)));
    break;
  }
  case ECesiumMetadataComponentType::Uint8:
  case ECesiumMetadataComponentType::Uint16:
  case ECesiumMetadataComponentType::Uint32:
  case ECesiumMetadataComponentType::Uint64: {
    std::vector<uint64_t> values = getUint64Array(array);
    resultArray = FCesiumPropertyArray(
        CesiumGltf::PropertyArrayCopy<uint64_t>(std::move(values)));

    break;
  }
  default:
    return;
  }

  // Override the element type. The element type is cached for user information
  // and does not impact the underlying element access.
  resultArray._elementType.ComponentType = targetType.ComponentType;
  this->_arrayValue = std::move(resultArray);
  this->_valueType = targetType;
}

void FCesiumMetadataValue::initializeAsVecOrMat(
    const CesiumUtility::JsonValue::Array& array,
    const FCesiumMetadataValueType& targetType) {
  ComponentTypeFit bestFit = getBestComponentTypeFit(array);
  if (!bestFit.isCompatibleWithComponentType(targetType.ComponentType))
    return;

  switch (targetType.ComponentType) {
  case ECesiumMetadataComponentType::Float32:
  case ECesiumMetadataComponentType::Float64: {
    std::vector<double> values = getDoubleArray(array);
    switch (targetType.Type) {
    case ECesiumMetadataType::Vec2:
      if (values.size() != 2)
        return;
      this->_value = *reinterpret_cast<glm::dvec2*>(values.data());
      break;
    case ECesiumMetadataType::Vec3:
      if (values.size() != 3)
        return;
      this->_value = *reinterpret_cast<glm::dvec3*>(values.data());
      break;
    case ECesiumMetadataType::Vec4:
      if (values.size() != 4)
        return;
      this->_value = *reinterpret_cast<glm::dvec4*>(values.data());
      break;
    case ECesiumMetadataType::Mat2:
      if (values.size() != 4)
        return;
      this->_value = *reinterpret_cast<glm::dmat2*>(values.data());
      break;
    case ECesiumMetadataType::Mat3:
      if (values.size() != 9)
        return;
      this->_value = *reinterpret_cast<glm::dmat3*>(values.data());
      break;
    case ECesiumMetadataType::Mat4:
      if (values.size() != 16)
        return;
      this->_value = *reinterpret_cast<glm::dmat4*>(values.data());
      break;
    default:
      return;
    }
    break;
  }
  case ECesiumMetadataComponentType::Int8:
  case ECesiumMetadataComponentType::Int16:
  case ECesiumMetadataComponentType::Int32:
  case ECesiumMetadataComponentType::Int64: {
    std::vector<int64_t> values = getInt64Array(array);
    switch (targetType.Type) {
    case ECesiumMetadataType::Vec2:
      if (values.size() != 2)
        return;
      this->_value = *reinterpret_cast<glm::vec<2, int64_t>*>(values.data());
      break;
    case ECesiumMetadataType::Vec3:
      if (values.size() != 3)
        return;
      this->_value = *reinterpret_cast<glm::vec<2, int64_t>*>(values.data());
      break;
    case ECesiumMetadataType::Vec4:
      if (values.size() != 4)
        return;
      this->_value = *reinterpret_cast<glm::vec<2, int64_t>*>(values.data());
      break;
    case ECesiumMetadataType::Mat2:
      if (values.size() != 4)
        return;
      this->_value = *reinterpret_cast<glm::mat<2, 2, int64_t>*>(values.data());
      break;
    case ECesiumMetadataType::Mat3:
      if (values.size() != 9)
        return;
      this->_value = *reinterpret_cast<glm::mat<3, 3, int64_t>*>(values.data());
      break;
    case ECesiumMetadataType::Mat4:
      if (values.size() != 16)
        return;
      this->_value = *reinterpret_cast<glm::mat<4, 4, int64_t>*>(values.data());
      break;
    default:
      return;
    }
    break;
  }
  case ECesiumMetadataComponentType::Uint8:
  case ECesiumMetadataComponentType::Uint16:
  case ECesiumMetadataComponentType::Uint32:
  case ECesiumMetadataComponentType::Uint64: {
    std::vector<uint64_t> values = getUint64Array(array);
    switch (targetType.Type) {
    case ECesiumMetadataType::Vec2:
      if (values.size() != 2)
        return;
      this->_value = *reinterpret_cast<glm::vec<2, uint64_t>*>(values.data());
      break;
    case ECesiumMetadataType::Vec3:
      if (values.size() != 3)
        return;
      this->_value = *reinterpret_cast<glm::vec<2, uint64_t>*>(values.data());
      break;
    case ECesiumMetadataType::Vec4:
      if (values.size() != 4)
        return;
      this->_value = *reinterpret_cast<glm::vec<2, uint64_t>*>(values.data());
      break;
    case ECesiumMetadataType::Mat2:
      if (values.size() != 4)
        return;
      this->_value =
          *reinterpret_cast<glm::mat<2, 2, uint64_t>*>(values.data());
      break;
    case ECesiumMetadataType::Mat3:
      if (values.size() != 9)
        return;
      this->_value =
          *reinterpret_cast<glm::mat<3, 3, uint64_t>*>(values.data());
      break;
    case ECesiumMetadataType::Mat4:
      if (values.size() != 16)
        return;
      this->_value =
          *reinterpret_cast<glm::mat<4, 4, uint64_t>*>(values.data());
      break;
    default:
      return;
    }
    break;
  }
  default:
    return;
  }

  // Override the value type. The value type struct is user-facing but does
  // not impact underlying element conversion or access.
  this->_valueType = targetType;
}

void FCesiumMetadataValue::initializeAsVecOrMatArray(
    const CesiumUtility::JsonValue::Array& array,
    const FCesiumMetadataValueType& targetType) {
  if (array.empty() || !array[0].isArray()) {
    return;
  }

  ComponentTypeFit bestFit = getBestComponentTypeFit(array[0].getArray());
  for (size_t i = 1; i < array.size(); i++) {
    if (!array[i].isArray()) {
      return;
    }
    bestFit &= getBestComponentTypeFit(array[i].getArray());
  }

  if (!bestFit.isCompatibleWithComponentType(targetType.ComponentType))
    return;

  const size_t expectedElementCount =
      CesiumGltf::getComponentCountFromPropertyType(
          CesiumGltf::PropertyType(targetType.Type));

  switch (targetType.ComponentType) {
  case ECesiumMetadataComponentType::Float32:
  case ECesiumMetadataComponentType::Float64: {
    std::vector<double> values;
    for (size_t i = 0; i < array.size(); i++) {
      std::vector<double> element = getDoubleArray(array[i].getArray());
      if (element.size() != expectedElementCount)
        return;
      values.insert(values.end(), element.begin(), element.end());
    }
    switch (targetType.Type) {
    case ECesiumMetadataType::Vec2: {
      std::vector<glm::dvec2> copy(array.size());
      std::memcpy(copy.data(), values.data(), values.size() * sizeof(double));
      this->_arrayValue = FCesiumPropertyArray(
          CesiumGltf::PropertyArrayCopy<glm::dvec2>(std::move(copy)));
      break;
    }
    case ECesiumMetadataType::Vec3: {
      std::vector<glm::dvec3> copy(array.size());
      std::memcpy(copy.data(), values.data(), values.size() * sizeof(double));
      this->_arrayValue = FCesiumPropertyArray(
          CesiumGltf::PropertyArrayCopy<glm::dvec3>(std::move(copy)));
      break;
    }
    case ECesiumMetadataType::Vec4: {
      std::vector<glm::dvec4> copy(array.size());
      std::memcpy(copy.data(), values.data(), values.size() * sizeof(double));
      this->_arrayValue = FCesiumPropertyArray(
          CesiumGltf::PropertyArrayCopy<glm::dvec4>(std::move(copy)));
      break;
    }
    case ECesiumMetadataType::Mat2: {
      std::vector<glm::dmat2> copy(array.size());
      std::memcpy(copy.data(), values.data(), values.size() * sizeof(double));
      this->_arrayValue = FCesiumPropertyArray(
          CesiumGltf::PropertyArrayCopy<glm::dmat2>(std::move(copy)));
      break;
    }
    case ECesiumMetadataType::Mat3: {
      std::vector<glm::dmat3> copy(array.size());
      std::memcpy(copy.data(), values.data(), values.size() * sizeof(double));
      this->_arrayValue = FCesiumPropertyArray(
          CesiumGltf::PropertyArrayCopy<glm::dmat3>(std::move(copy)));
      break;
    }
    case ECesiumMetadataType::Mat4: {
      std::vector<glm::dmat4> copy(array.size());
      std::memcpy(copy.data(), values.data(), values.size() * sizeof(double));
      this->_arrayValue = FCesiumPropertyArray(
          CesiumGltf::PropertyArrayCopy<glm::dmat4>(std::move(copy)));
      break;
    }
    default:
      return;
    }
    break;
  }
  case ECesiumMetadataComponentType::Int8:
  case ECesiumMetadataComponentType::Int16:
  case ECesiumMetadataComponentType::Int32:
  case ECesiumMetadataComponentType::Int64: {
    std::vector<int64_t> values;
    for (size_t i = 0; i < array.size(); i++) {
      std::vector<int64_t> element = getInt64Array(array[i].getArray());
      if (element.size() != expectedElementCount)
        return;
      values.insert(values.end(), element.begin(), element.end());
    }
    switch (targetType.Type) {
    case ECesiumMetadataType::Vec2: {
      std::vector<glm::vec<2, int64_t>> copy(array.size());
      std::memcpy(copy.data(), values.data(), values.size() * sizeof(int64_t));
      this->_arrayValue = FCesiumPropertyArray(
          CesiumGltf::PropertyArrayCopy<glm::vec<2, int64_t>>(std::move(copy)));
      break;
    }
    case ECesiumMetadataType::Vec3: {
      std::vector<glm::vec<3, int64_t>> copy(array.size());
      std::memcpy(copy.data(), values.data(), values.size() * sizeof(int64_t));
      this->_arrayValue = FCesiumPropertyArray(
          CesiumGltf::PropertyArrayCopy<glm::vec<3, int64_t>>(std::move(copy)));
      break;
    }
    case ECesiumMetadataType::Vec4: {
      std::vector<glm::vec<4, int64_t>> copy(array.size());
      std::memcpy(copy.data(), values.data(), values.size() * sizeof(int64_t));
      this->_arrayValue = FCesiumPropertyArray(
          CesiumGltf::PropertyArrayCopy<glm::vec<4, int64_t>>(std::move(copy)));
      break;
    }
    case ECesiumMetadataType::Mat2: {
      std::vector<glm::mat<2, 2, int64_t>> copy(array.size());
      std::memcpy(copy.data(), values.data(), values.size() * sizeof(int64_t));
      this->_arrayValue = FCesiumPropertyArray(
          CesiumGltf::PropertyArrayCopy<glm::mat<2, 2, int64_t>>(
              std::move(copy)));
      break;
    }
    case ECesiumMetadataType::Mat3: {
      std::vector<glm::mat<3, 3, int64_t>> copy(array.size());
      std::memcpy(copy.data(), values.data(), values.size() * sizeof(int64_t));
      this->_arrayValue = FCesiumPropertyArray(
          CesiumGltf::PropertyArrayCopy<glm::mat<3, 3, int64_t>>(
              std::move(copy)));
      break;
    }
    case ECesiumMetadataType::Mat4: {
      std::vector<glm::mat<4, 4, int64_t>> copy(array.size());
      std::memcpy(copy.data(), values.data(), values.size() * sizeof(int64_t));
      this->_arrayValue = FCesiumPropertyArray(
          CesiumGltf::PropertyArrayCopy<glm::mat<4, 4, int64_t>>(
              std::move(copy)));
      break;
    }
    default:
      return;
    }
    break;
  }
  case ECesiumMetadataComponentType::Uint8:
  case ECesiumMetadataComponentType::Uint16:
  case ECesiumMetadataComponentType::Uint32:
  case ECesiumMetadataComponentType::Uint64: {
    std::vector<uint64_t> values;
    for (size_t i = 0; i < array.size(); i++) {
      std::vector<uint64_t> element = getUint64Array(array[i].getArray());
      if (element.size() != expectedElementCount)
        return;
      values.insert(values.end(), element.begin(), element.end());
    }
    switch (targetType.Type) {
    case ECesiumMetadataType::Vec2: {
      std::vector<glm::vec<2, uint64_t>> copy(array.size());
      std::memcpy(copy.data(), values.data(), values.size() * sizeof(uint64_t));
      this->_arrayValue = FCesiumPropertyArray(
          CesiumGltf::PropertyArrayCopy<glm::vec<2, uint64_t>>(
              std::move(copy)));
      break;
    }
    case ECesiumMetadataType::Vec3: {
      std::vector<glm::vec<3, uint64_t>> copy(array.size());
      std::memcpy(copy.data(), values.data(), values.size() * sizeof(uint64_t));
      this->_arrayValue = FCesiumPropertyArray(
          CesiumGltf::PropertyArrayCopy<glm::vec<3, uint64_t>>(
              std::move(copy)));
      break;
    }
    case ECesiumMetadataType::Vec4: {
      std::vector<glm::vec<4, uint64_t>> copy(array.size());
      std::memcpy(copy.data(), values.data(), values.size() * sizeof(uint64_t));
      this->_arrayValue = FCesiumPropertyArray(
          CesiumGltf::PropertyArrayCopy<glm::vec<4, uint64_t>>(
              std::move(copy)));
      break;
    }
    case ECesiumMetadataType::Mat2: {
      std::vector<glm::mat<2, 2, uint64_t>> copy(array.size());
      std::memcpy(copy.data(), values.data(), values.size() * sizeof(uint64_t));
      this->_arrayValue = FCesiumPropertyArray(
          CesiumGltf::PropertyArrayCopy<glm::mat<2, 2, uint64_t>>(
              std::move(copy)));
      break;
    }
    case ECesiumMetadataType::Mat3: {
      std::vector<glm::mat<3, 3, uint64_t>> copy(array.size());
      std::memcpy(copy.data(), values.data(), values.size() * sizeof(uint64_t));
      this->_arrayValue = FCesiumPropertyArray(
          CesiumGltf::PropertyArrayCopy<glm::mat<3, 3, uint64_t>>(
              std::move(copy)));
      break;
    }
    case ECesiumMetadataType::Mat4: {
      std::vector<glm::mat<4, 4, uint64_t>> copy(array.size());
      std::memcpy(copy.data(), values.data(), values.size() * sizeof(uint64_t));
      this->_arrayValue = FCesiumPropertyArray(
          CesiumGltf::PropertyArrayCopy<glm::mat<4, 4, uint64_t>>(
              std::move(copy)));
      break;
    }
    default:
      return;
    }
    break;
  }
  default:
    return;
  }

  // Override the value type. The value type struct is user-facing but does
  // not impact underlying element conversion or access.
  this->_valueType = targetType;
}

void FCesiumMetadataValue::initializeFromJsonArray(
    const CesiumUtility::JsonValue::Array& array,
    const FCesiumMetadataValueType& targetType) {
  if (array.empty()) {
    return;
  }

  if (!targetType.bIsArray) {
    // If the target type is not actually an array, then it must be a vector or
    // matrix value.
    this->initializeAsVecOrMat(array, targetType);
    return;
  }

  switch (targetType.Type) {
  case ECesiumMetadataType::Boolean: {
    std::vector<bool> values(array.size());
    for (size_t i = 0; i < array.size(); i++) {
      if (!array[i].isBool()) {
        return;
      }
      values[i] = array[i].getBool();
    }
    this->_arrayValue =
        FCesiumPropertyArray(CesiumGltf::PropertyArrayCopy<bool>(values));
    break;
  }
  case ECesiumMetadataType::String: {
    std::vector<std::string> values(array.size());
    for (size_t i = 0; i < array.size(); i++) {
      if (!array[i].isString()) {
        return;
      }
      values[i] = array[i].getString();
    }
    this->_arrayValue = FCesiumPropertyArray(
        CesiumGltf::PropertyArrayCopy<std::string_view>(values));
    break;
  }
  case ECesiumMetadataType::Scalar:
  case ECesiumMetadataType::Enum:
    this->initializeAsScalarArray(array, targetType);
    break;
  case ECesiumMetadataType::Vec2:
  case ECesiumMetadataType::Vec3:
  case ECesiumMetadataType::Vec4:
  case ECesiumMetadataType::Mat2:
  case ECesiumMetadataType::Mat3:
  case ECesiumMetadataType::Mat4:
    this->initializeAsVecOrMatArray(array, targetType);
    break;
  default:
    return;
  }
}

FCesiumMetadataValue::FCesiumMetadataValue(
    const CesiumUtility::JsonValue& JsonValue,
    const FCesiumMetadataValueType& TargetType)
    : _value(), _arrayValue(), _valueType(), _pEnumDefinition(nullptr) {
  if (JsonValue.isArray()) {
    this->initializeFromJsonArray(JsonValue.getArray(), TargetType);
    return;
  } else if (TargetType.bIsArray) {
    // Array type mismatch; exit early.
    return;
  }

  switch (TargetType.Type) {
  case ECesiumMetadataType::Boolean:
    if (JsonValue.isBool()) {
      this->_value = JsonValue.getBool();
      this->_valueType.Type = ECesiumMetadataType::Boolean;
    }
    return;
  case ECesiumMetadataType::String:
    if (JsonValue.isString()) {
      this->_value = std::string_view(JsonValue.getString());
      this->_valueType.Type = ECesiumMetadataType::String;
    }
    return;
  case ECesiumMetadataType::Scalar:
  case ECesiumMetadataType::Enum: {
    ComponentTypeFit bestFit = getBestComponentTypeFit(JsonValue);
    if (!bestFit.isCompatibleWithComponentType(TargetType.ComponentType)) {
      return;
    }

    if (JsonValue.isInt64()) {
      this->_value = JsonValue.getInt64();
    } else if (JsonValue.isUint64()) {
      this->_value = JsonValue.getUint64();
    } else if (JsonValue.isDouble()) {
      this->_value = JsonValue.getDouble();
    }
    this->_valueType = TargetType;
    return;
  }
  default:
    return;
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
