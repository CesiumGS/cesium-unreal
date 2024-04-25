// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumFeatureIdAttribute.h"
#include <CesiumGltf/Accessor.h>
#include <CesiumGltf/Model.h>

using namespace CesiumGltf;

FCesiumFeatureIdAttribute::FCesiumFeatureIdAttribute(
    const Model& Model,
    const MeshPrimitive& Primitive,
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

  const Accessor* accessor =
      Model.getSafe<Accessor>(&Model.accessors, featureID->second);
  if (!accessor || accessor->type != Accessor::Type::SCALAR) {
    this->_status = ECesiumFeatureIdAttributeStatus::ErrorInvalidAccessor;
    return;
  }

  this->_featureIdAccessor = CesiumGltf::getFeatureIdAccessorView(
      Model,
      Primitive,
      this->_attributeIndex);

  this->_status = std::visit(
      [](auto view) {
        if (view.status() != AccessorViewStatus::Valid) {
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
  return std::visit(CountFromAccessor{}, FeatureIDAttribute._featureIdAccessor);
}

int64 UCesiumFeatureIdAttributeBlueprintLibrary::GetFeatureIDForVertex(
    UPARAM(ref) const FCesiumFeatureIdAttribute& FeatureIDAttribute,
    int64 VertexIndex) {
  return std::visit(
      FeatureIdFromAccessor{VertexIndex},
      FeatureIDAttribute._featureIdAccessor);
}
