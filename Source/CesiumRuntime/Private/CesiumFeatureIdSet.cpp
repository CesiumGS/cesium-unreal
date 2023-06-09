// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumFeatureIdSet.h"
#include "CesiumFeatureTable.h"
#include "CesiumGltf/Accessor.h"
#include "CesiumGltf/ExtensionExtMeshFeaturesFeatureId.h"
#include "CesiumGltf/ExtensionModelExtStructuralMetadata.h"
#include "CesiumGltf/Model.h"

using namespace CesiumGltf;

static FCesiumFeatureIdAttribute EmptyFeatureIDAttribute;
static FCesiumFeatureIdTexture EmptyFeatureIDTexture;

FCesiumFeatureIdSet::FCesiumFeatureIdSet(
    const Model& InModel,
    const MeshPrimitive& Primitive,
    const ExtensionExtMeshFeaturesFeatureId& FeatureID)
    : _featureID(),
      _featureIDType(ECesiumFeatureIdType::None),
      _featureCount(FeatureID.featureCount),
      _nullFeatureID(FeatureID.nullFeatureId ? *FeatureID.nullFeatureId : -1),
      _propertyTableIndex() {
  FString propertyTableName;

  if (FeatureID.propertyTable) {
    _propertyTableIndex = *FeatureID.propertyTable;
  }

  // For backwards compatibility with GetFeatureTableName.
  const ExtensionModelExtStructuralMetadata* pMetadata =
      InModel.getExtension<ExtensionModelExtStructuralMetadata>();
  if (pMetadata && _propertyTableIndex.IsSet() &&
      _propertyTableIndex.GetValue() >= 0 &&
      static_cast<size_t>(_propertyTableIndex.GetValue()) <
          pMetadata->propertyTables.size()) {
    const ExtensionExtStructuralMetadataPropertyTable& propertyTable =
        pMetadata->propertyTables[_propertyTableIndex.GetValue()];
    std::string name = propertyTable.name ? *propertyTable.name : "";
    propertyTableName = FString(name.c_str());
  }

  if (FeatureID.attribute) {
    _featureID = FCesiumFeatureIdAttribute(
        InModel,
        Primitive,
        *FeatureID.attribute,
        propertyTableName);
    _featureIDType = ECesiumFeatureIdType::Attribute;

    return;
  }

  if (FeatureID.texture) {
    _featureID = FCesiumFeatureIdTexture(
        InModel,
        Primitive,
        *FeatureID.texture,
        propertyTableName);
    _featureIDType = ECesiumFeatureIdType::Texture;

    return;
  }

  if (_featureCount > 0) {
    _featureIDType = ECesiumFeatureIdType::Implicit;
  }
}

const ECesiumFeatureIdType
UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDType(
    UPARAM(ref) const FCesiumFeatureIdSet& FeatureIDSet) {
  return FeatureIDSet._featureIDType;
}

const FCesiumFeatureIdAttribute&
UCesiumFeatureIdSetBlueprintLibrary::GetAsFeatureIDAttribute(
    UPARAM(ref) const FCesiumFeatureIdSet& FeatureIDSet) {
  if (FeatureIDSet._featureIDType == ECesiumFeatureIdType::Attribute) {
    return std::get<FCesiumFeatureIdAttribute>(FeatureIDSet._featureID);
  }

  return EmptyFeatureIDAttribute;
}

const FCesiumFeatureIdTexture&
UCesiumFeatureIdSetBlueprintLibrary::GetAsFeatureIDTexture(
    UPARAM(ref) const FCesiumFeatureIdSet& FeatureIDSet) {
  if (FeatureIDSet._featureIDType == ECesiumFeatureIdType::Texture) {
    return std::get<FCesiumFeatureIdTexture>(FeatureIDSet._featureID);
  }

  return EmptyFeatureIDTexture;
}

const int64 UCesiumFeatureIdSetBlueprintLibrary::GetPropertyTableIndex(
    UPARAM(ref) const FCesiumFeatureIdSet& FeatureIDSet) {
  return FeatureIDSet._propertyTableIndex.IsSet()
             ? FeatureIDSet._propertyTableIndex.GetValue()
             : -1;
}

int64 UCesiumFeatureIdSetBlueprintLibrary::GetFeatureCount(
    UPARAM(ref) const FCesiumFeatureIdSet& FeatureIDSet) {
  return FeatureIDSet._featureCount;
}

const int64 UCesiumFeatureIdSetBlueprintLibrary::GetNullFeatureID(
    UPARAM(ref) const FCesiumFeatureIdSet& FeatureIDSet) {
  return FeatureIDSet._nullFeatureID;
}

int64 UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDForVertex(
    UPARAM(ref) const FCesiumFeatureIdSet& FeatureIDSet,
    int64 VertexIndex) {
  if (FeatureIDSet._featureIDType == ECesiumFeatureIdType::Attribute) {
    FCesiumFeatureIdAttribute attribute =
        std::get<FCesiumFeatureIdAttribute>(FeatureIDSet._featureID);
    return UCesiumFeatureIdAttributeBlueprintLibrary::GetFeatureIDForVertex(
        attribute,
        VertexIndex);
  }

  if (FeatureIDSet._featureIDType == ECesiumFeatureIdType::Texture) {
    FCesiumFeatureIdTexture texture =
        std::get<FCesiumFeatureIdTexture>(FeatureIDSet._featureID);
    return UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDForVertex(
        texture,
        VertexIndex);
  }

  if (FeatureIDSet._featureIDType == ECesiumFeatureIdType::Implicit) {
    return (VertexIndex >= 0 && VertexIndex < FeatureIDSet._featureCount)
               ? VertexIndex
               : -1;
  }

  return -1;
}
