// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumFeatureIdAttribute.h"
#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/ExtensionExtMeshGpuInstancing.h>
#include <CesiumGltf/Model.h>

FCesiumFeatureIdAttribute::FCesiumFeatureIdAttribute(
    const CesiumGltf::Model& Model,
    const CesiumGltf::MeshPrimitive& Primitive,
    const int64 FeatureIDAttribute,
    const FString& PropertyTableName)
    : _status(ECesiumFeatureIdAttributeStatus::ErrorInvalidAttribute),
      _featureIdAccessor(),
      _attributeIndex(FeatureIDAttribute),
      _propertyTableName(PropertyTableName) {
  const std::string attributeName =
      "_FEATURE_ID_" + std::to_string(FeatureIDAttribute);

  auto featureID = Primitive.attributes.find(attributeName);
  if (featureID == Primitive.attributes.end()) {
    return;
  }

  const CesiumGltf::Accessor* accessor =
      Model.getSafe<CesiumGltf::Accessor>(&Model.accessors, featureID->second);
  if (!accessor || accessor->type != CesiumGltf::Accessor::Type::SCALAR) {
    this->_status = ECesiumFeatureIdAttributeStatus::ErrorInvalidAccessor;
    return;
  }

  this->_featureIdAccessor = CesiumGltf::getFeatureIdAccessorView(
      Model,
      Primitive,
      this->_attributeIndex);

  this->_status = std::visit(
      [](auto view) {
        if (view.status() != CesiumGltf::AccessorViewStatus::Valid) {
          return ECesiumFeatureIdAttributeStatus::ErrorInvalidAccessor;
        }

        return ECesiumFeatureIdAttributeStatus::Valid;
      },
      this->_featureIdAccessor);
}

FCesiumFeatureIdAttribute::FCesiumFeatureIdAttribute(
    const CesiumGltf::Model& Model,
    const CesiumGltf::Node& Node,
    const int64 FeatureIDAttribute,
    const FString& PropertyTableName)
    : _status(ECesiumFeatureIdAttributeStatus::ErrorInvalidAttribute),
      _featureIdAccessor(),
      _attributeIndex(FeatureIDAttribute),
      _propertyTableName(PropertyTableName) {
  this->_featureIdAccessor = CesiumGltf::getFeatureIdAccessorView(
      Model,
      Node,
      static_cast<int32_t>(this->_attributeIndex));

  this->_status = std::visit(
      [](auto&& view) {
        if (view.status() != CesiumGltf::AccessorViewStatus::Valid) {
          return ECesiumFeatureIdAttributeStatus::ErrorInvalidAccessor;
        }

        return ECesiumFeatureIdAttributeStatus::Valid;
      },
      this->_featureIdAccessor);
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
      CesiumGltf::CountFromAccessor{},
      FeatureIDAttribute._featureIdAccessor);
}

int64 UCesiumFeatureIdAttributeBlueprintLibrary::GetFeatureIDForVertex(
    UPARAM(ref) const FCesiumFeatureIdAttribute& FeatureIDAttribute,
    int64 VertexIndex) {
  return std::visit(
      CesiumGltf::FeatureIdFromAccessor{VertexIndex},
      FeatureIDAttribute._featureIdAccessor);
}
