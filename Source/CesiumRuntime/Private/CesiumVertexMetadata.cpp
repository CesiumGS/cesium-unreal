// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumVertexMetadata.h"
#include "CesiumGltf/Accessor.h"
#include "CesiumGltf/FeatureTable.h"
#include "CesiumGltf/Model.h"
#include "CesiumMetadataFeatureTable.h"

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

} // namespace

FCesiumVertexMetadata::FCesiumVertexMetadata(
    const Model& model,
    const Accessor& featureIDAccessor,
    const FString& featureTableName)
    : _featureTableName(featureTableName) {

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

const FString& UCesiumVertexMetadataBlueprintLibrary::GetFeatureTableName(
    UPARAM(ref) const FCesiumVertexMetadata& VertexMetadata) {
  return VertexMetadata._featureTableName;
}

int64 UCesiumVertexMetadataBlueprintLibrary::GetFeatureIDForVertex(
    UPARAM(ref) const FCesiumVertexMetadata& VertexMetadata,
    int64 vertexIdx) {
  return std::visit(
      FeatureIDFromAccessor{vertexIdx},
      VertexMetadata._featureIDAccessor);
}