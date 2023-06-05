// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumFeatureIdSet.h"
#include "CesiumFeatureTable.h"
#include "CesiumGltf/Accessor.h"
#include "CesiumGltf/ExtensionExtMeshFeaturesFeatureId.h"
#include "CesiumGltf/Model.h"

using namespace CesiumGltf;

static FCesiumFeatureIDAttribute EmptyFeatureIDAttribute;
static FCesiumFeatureIDTexture EmptyFeatureIDTexture;

FCesiumFeatureIDSet::FCesiumFeatureIDSet(
    const Model& InModel,
    const MeshPrimitive& Primitive,
    const ExtensionExtMeshFeaturesFeatureId& FeatureID)
    : _featureID(),
      _featureIDType(ECesiumFeatureIDType::None),
      _featureCount(FeatureID.featureCount),
      _propertyTableIndex() {
  if (FeatureID.propertyTable) {
    _propertyTableIndex = *FeatureID.propertyTable;
  }

  if (FeatureID.attribute) {
    _featureID =
        FCesiumFeatureIDAttribute(InModel, Primitive, *FeatureID.attribute);
    _featureIDType = ECesiumFeatureIDType::Attribute;

    return;
  }

  if (FeatureID.texture) {
    _featureID =
        FCesiumFeatureIDTexture(InModel, Primitive, *FeatureID.texture);
    _featureIDType = ECesiumFeatureIDType::Texture;

    return;
  }

  if (_featureCount > 0) {
    _featureIDType = ECesiumFeatureIDType::Implicit;
  }
}

const ECesiumFeatureIDType
UCesiumFeatureIDSetBlueprintLibrary::GetFeatureIDType(
    UPARAM(ref) const FCesiumFeatureIDSet& FeatureID) {
  return FeatureID._featureIDType;
}

const FCesiumFeatureIDAttribute
UCesiumFeatureIDSetBlueprintLibrary::GetAsFeatureIDAttribute(
    UPARAM(ref) const FCesiumFeatureIDSet& FeatureID) {
  if (FeatureID._featureIDType == ECesiumFeatureIDType::Attribute) {
    return std::get<FCesiumFeatureIDAttribute>(FeatureID._featureID);
  }

  return EmptyFeatureIDAttribute;
}

const FCesiumFeatureIDTexture
UCesiumFeatureIDSetBlueprintLibrary::GetAsFeatureIDTexture(
    UPARAM(ref) const FCesiumFeatureIDSet& FeatureID) {
  if (FeatureID._featureIDType == ECesiumFeatureIDType::Texture) {
    return std::get<FCesiumFeatureIDTexture>(FeatureID._featureID);
  }

  return EmptyFeatureIDTexture;
}

const int64 UCesiumFeatureIDSetBlueprintLibrary::GetPropertyTableIndex(
    UPARAM(ref) const FCesiumFeatureIDSet& FeatureIDSet) {
  return FeatureIDSet._propertyTableIndex.IsSet()
             ? FeatureIDSet._propertyTableIndex.GetValue()
             : -1;
}

int64 UCesiumFeatureIDSetBlueprintLibrary::GetFeatureCount(
    UPARAM(ref) const FCesiumFeatureIDSet& FeatureIDSet) {
  return FeatureIDSet._featureCount;
}

int64 UCesiumFeatureIDSetBlueprintLibrary::GetFeatureIDForVertex(
    UPARAM(ref) const FCesiumFeatureIDSet& FeatureIDSet,
    int64 VertexIndex) {
  if (FeatureIDSet._featureIDType == ECesiumFeatureIDType::Attribute) {
    FCesiumFeatureIDAttribute attribute =
        std::get<FCesiumFeatureIDAttribute>(FeatureIDSet._featureID);
    return UCesiumFeatureIDAttributeBlueprintLibrary::GetFeatureIDForVertex(
        attribute,
        VertexIndex);
  }

  if (FeatureIDSet._featureIDType == ECesiumFeatureIDType::Texture) {
    FCesiumFeatureIDTexture texture =
        std::get<FCesiumFeatureIDTexture>(FeatureIDSet._featureID);
    return UCesiumFeatureIDTextureBlueprintLibrary::GetFeatureIDForVertex(
        texture,
        VertexIndex);
  }

  if (FeatureIDSet._featureIDType == ECesiumFeatureIDType::Implicit) {
    return VertexIndex;
  }

  return -1;
}
