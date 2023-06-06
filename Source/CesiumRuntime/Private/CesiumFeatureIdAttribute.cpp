// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumFeatureIdAttribute.h"
#include "CesiumFeatureTable.h"
#include "CesiumGltf/Accessor.h"
#include "CesiumGltf/FeatureTable.h"
#include "CesiumGltf/Model.h"

using namespace CesiumGltf;

namespace {

struct FeatureIDFromAccessor {
  int64 operator()(std::monostate) { return -1; }

  int64 operator()(
      const AccessorView<CesiumGltf::AccessorTypes::SCALAR<float>>& value) {
    return static_cast<int64>(glm::round(value[vertexIdx].value[0]));
  }

  template <typename T> int64 operator()(const AccessorView<T>& value) {
    return static_cast<int64>(value[vertexIdx].value[0]);
  }

  int64 vertexIdx;
};

struct VertexCountFromAccessor {
  int64 operator()(std::monostate) { return 0; }

  template <typename T> int64 operator()(const AccessorView<T>& value) {
    return static_cast<int64>(value.size());
  }
};
} // namespace

FCesiumFeatureIdAttribute::FCesiumFeatureIdAttribute(
    const Model& model,
    const CesiumGltf::MeshPrimitive Primitive,
    const int64 FeatureIDAttribute)
    : _attributeIndex() {
  const std::string attributeName =
      "_FEATURE_ID_" + std::to_string(FeatureIDAttribute);

  auto featureID = Primitive.attributes.find(attributeName);
  if (featureID == Primitive.attributes.end()) {
    return;
  }

  const Accessor* accessor =
      model.getSafe<Accessor>(&model.accessors, featureID->second);
  if (!accessor || accessor->type != Accessor::Type::SCALAR) {
    return;
  }

  switch (accessor->componentType) {
  case CesiumGltf::Accessor::ComponentType::BYTE:
    this->_featureIDAccessor =
        CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<int8_t>>(
            model,
            *accessor);
    break;
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE:
    this->_featureIDAccessor =
        CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<uint8_t>>(
            model,
            *accessor);
    break;
  case CesiumGltf::Accessor::ComponentType::SHORT:
    this->_featureIDAccessor =
        CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<int16_t>>(
            model,
            *accessor);
    break;
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT:
    this->_featureIDAccessor =
        CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<uint16_t>>(
            model,
            *accessor);
    break;
  case CesiumGltf::Accessor::ComponentType::FLOAT:
    this->_featureIDAccessor =
        CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<float>>(
            model,
            *accessor);
    break;
  default:
    break;
  }
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
  if (VertexIndex < 0 || VertexIndex >= GetVertexCount(FeatureIDAttribute)) {
    return -1;
  }

  return std::visit(
      FeatureIDFromAccessor{VertexIndex},
      FeatureIDAttribute._featureIDAccessor);
}
