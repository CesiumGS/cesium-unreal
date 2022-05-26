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
    const Accessor& featureIDAccessor,
    int32 attributeIndex,
    const FString& featureTableName)
    : _featureTableName(featureTableName), _attributeIndex(attributeIndex) {
  switch (featureIDAccessor.componentType) {
  case CesiumGltf::Accessor::ComponentType::BYTE:
    this->_featureIDAccessor =
        CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<int8_t>>(
            model,
            featureIDAccessor);
    break;
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE:
    this->_featureIDAccessor =
        CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<uint8_t>>(
            model,
            featureIDAccessor);
    break;
  case CesiumGltf::Accessor::ComponentType::SHORT:
    this->_featureIDAccessor =
        CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<int16_t>>(
            model,
            featureIDAccessor);
    break;
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT:
    this->_featureIDAccessor =
        CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<uint16_t>>(
            model,
            featureIDAccessor);
    break;
  case CesiumGltf::Accessor::ComponentType::FLOAT:
    this->_featureIDAccessor =
        CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<float>>(
            model,
            featureIDAccessor);
    break;
  default:
    break;
  }
}

const FString& UCesiumFeatureIdAttributeBlueprintLibrary::GetFeatureTableName(
    UPARAM(ref) const FCesiumFeatureIdAttribute& FeatureIdAttribute) {
  return FeatureIdAttribute._featureTableName;
}

int64 UCesiumFeatureIdAttributeBlueprintLibrary::GetVertexCount(
    UPARAM(ref) const FCesiumFeatureIdAttribute& FeatureIdAttribute) {
  return std::visit(
      VertexCountFromAccessor{},
      FeatureIdAttribute._featureIDAccessor);
}

int64 UCesiumFeatureIdAttributeBlueprintLibrary::GetFeatureIDForVertex(
    UPARAM(ref) const FCesiumFeatureIdAttribute& FeatureIdAttribute,
    int64 vertexIdx) {
  return std::visit(
      FeatureIDFromAccessor{vertexIdx},
      FeatureIdAttribute._featureIDAccessor);
}
