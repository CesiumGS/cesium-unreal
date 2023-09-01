// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumFeatureIdAttribute.h"
#include "CesiumGltf/Accessor.h"
#include "CesiumGltf/Model.h"

using namespace CesiumGltf;

namespace {

struct FeatureIDFromAccessor {
  int64 operator()(std::monostate) { return -1; }

  int64 operator()(const AccessorView<float>& value) {
    if (vertexIndex < 0 || vertexIndex >= value.size()) {
      return -1;
    }
    return static_cast<int64>(glm::round(value[vertexIndex]));
  }

  template <typename T> int64 operator()(const AccessorView<T>& value) {
    if (vertexIndex < 0 || vertexIndex >= value.size()) {
      return -1;
    }
    return static_cast<int64>(value[vertexIndex]);
  }

  int64 vertexIndex;
};

struct VertexCountFromAccessor {
  int64 operator()(std::monostate) { return 0; }

  template <typename T> int64 operator()(const AccessorView<T>& value) {
    return static_cast<int64>(value.size());
  }
};
} // namespace

FCesiumFeatureIdAttribute::FCesiumFeatureIdAttribute(
    const Model& Model,
    const MeshPrimitive& Primitive,
    const int64 FeatureIDAttribute,
    const FString& PropertyTableName)
    : _status(ECesiumFeatureIdAttributeStatus::ErrorInvalidAttribute),
      _attributeIndex(FeatureIDAttribute),
      _propertyTableName(PropertyTableName) {
  const std::string attributeName =
      "_FEATURE_ID_" + std::to_string(FeatureIDAttribute);

  auto featureID = Primitive.attributes.find(attributeName);
  if (featureID == Primitive.attributes.end()) {
    return;
  }

  const Accessor* accessor =
      Model.getSafe<Accessor>(&Model.accessors, featureID->second);
  if (!accessor || accessor->type != Accessor::Type::SCALAR) {
    _status = ECesiumFeatureIdAttributeStatus::ErrorInvalidAccessor;
    return;
  }

  switch (accessor->componentType) {
  case Accessor::ComponentType::BYTE:
    this->_featureIDAccessor = AccessorView<int8_t>(Model, *accessor);
    break;
  case Accessor::ComponentType::UNSIGNED_BYTE:
    this->_featureIDAccessor = AccessorView<uint8_t>(Model, *accessor);
    break;
  case Accessor::ComponentType::SHORT:
    this->_featureIDAccessor = AccessorView<int16_t>(Model, *accessor);
    break;
  case Accessor::ComponentType::UNSIGNED_SHORT:
    this->_featureIDAccessor = AccessorView<uint16_t>(Model, *accessor);
    break;
  case Accessor::ComponentType::FLOAT:
    this->_featureIDAccessor = AccessorView<float>(Model, *accessor);
    break;
  default:
    _status = ECesiumFeatureIdAttributeStatus::ErrorInvalidAccessor;
    return;
  }

  _status = ECesiumFeatureIdAttributeStatus::Valid;
}

const FString& UCesiumFeatureIdAttributeBlueprintLibrary::GetFeatureTableName(
    UPARAM(ref) const FCesiumFeatureIdAttribute& FeatureIDAttribute) {
  return FeatureIDAttribute._propertyTableName;
}

ECesiumFeatureIdAttributeStatus
UCesiumFeatureIdAttributeBlueprintLibrary::GetFeatureIDAttributeStatus(
    UPARAM(ref) const FCesiumFeatureIdAttribute& FeatureIDAttribute) {
  return FeatureIDAttribute._status;
}

int64 UCesiumFeatureIdAttributeBlueprintLibrary::GetVertexCount(
    UPARAM(ref) const FCesiumFeatureIdAttribute& FeatureIDAttribute) {
  return std::visit(
      VertexCountFromAccessor{},
      FeatureIDAttribute._featureIDAccessor);
}

int64 UCesiumFeatureIdAttributeBlueprintLibrary::GetFeatureIDForVertex(
    UPARAM(ref) const FCesiumFeatureIdAttribute& FeatureIDAttribute,
    int64 VertexIndex) {
  return std::visit(
      FeatureIDFromAccessor{VertexIndex},
      FeatureIDAttribute._featureIDAccessor);
}
