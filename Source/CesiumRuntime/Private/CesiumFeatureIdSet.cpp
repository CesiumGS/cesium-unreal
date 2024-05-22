// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumFeatureIdSet.h"
#include "CesiumGltf/Accessor.h"
#include "CesiumGltf/ExtensionModelExtStructuralMetadata.h"
#include "CesiumGltf/FeatureId.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltfPrimitiveComponent.h"

using namespace CesiumGltf;

static FCesiumFeatureIdAttribute EmptyFeatureIDAttribute;
static FCesiumFeatureIdTexture EmptyFeatureIDTexture;

FCesiumFeatureIdSet::FCesiumFeatureIdSet(
    const Model& InModel,
    const MeshPrimitive& Primitive,
    const FeatureId& FeatureID)
    : _featureID(),
      _featureIDSetType(ECesiumFeatureIdSetType::None),
      _featureCount(FeatureID.featureCount),
      _nullFeatureID(FeatureID.nullFeatureId.value_or(-1)),
      _propertyTableIndex(FeatureID.propertyTable.value_or(-1)),
      _label(FString(FeatureID.label.value_or("").c_str())) {
  FString propertyTableName;

  // For backwards compatibility with GetFeatureTableName.
  const ExtensionModelExtStructuralMetadata* pMetadata =
      InModel.getExtension<ExtensionModelExtStructuralMetadata>();
  if (pMetadata && _propertyTableIndex >= 0) {
    size_t index = static_cast<size_t>(_propertyTableIndex);
    if (index < pMetadata->propertyTables.size()) {
      const PropertyTable& propertyTable = pMetadata->propertyTables[index];
      std::string name = propertyTable.name.value_or("");
      propertyTableName = FString(name.c_str());
    }
  }

  if (FeatureID.attribute) {
    _featureID = FCesiumFeatureIdAttribute(
        InModel,
        Primitive,
        *FeatureID.attribute,
        propertyTableName);
    _featureIDSetType = ECesiumFeatureIdSetType::Attribute;

    return;
  }

  if (FeatureID.texture) {
    _featureID = FCesiumFeatureIdTexture(
        InModel,
        Primitive,
        *FeatureID.texture,
        propertyTableName);
    _featureIDSetType = ECesiumFeatureIdSetType::Texture;

    return;
  }

  if (_featureCount > 0) {
    _featureIDSetType = ECesiumFeatureIdSetType::Implicit;
  }
}

const ECesiumFeatureIdSetType
UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDSetType(
    UPARAM(ref) const FCesiumFeatureIdSet& FeatureIDSet) {
  return FeatureIDSet._featureIDSetType;
}

const FCesiumFeatureIdAttribute&
UCesiumFeatureIdSetBlueprintLibrary::GetAsFeatureIDAttribute(
    UPARAM(ref) const FCesiumFeatureIdSet& FeatureIDSet) {
  if (FeatureIDSet._featureIDSetType == ECesiumFeatureIdSetType::Attribute) {
    return std::get<FCesiumFeatureIdAttribute>(FeatureIDSet._featureID);
  }

  return EmptyFeatureIDAttribute;
}

const FCesiumFeatureIdTexture&
UCesiumFeatureIdSetBlueprintLibrary::GetAsFeatureIDTexture(
    UPARAM(ref) const FCesiumFeatureIdSet& FeatureIDSet) {
  if (FeatureIDSet._featureIDSetType == ECesiumFeatureIdSetType::Texture) {
    return std::get<FCesiumFeatureIdTexture>(FeatureIDSet._featureID);
  }

  return EmptyFeatureIDTexture;
}

const int64 UCesiumFeatureIdSetBlueprintLibrary::GetPropertyTableIndex(
    UPARAM(ref) const FCesiumFeatureIdSet& FeatureIDSet) {
  return FeatureIDSet._propertyTableIndex;
}

int64 UCesiumFeatureIdSetBlueprintLibrary::GetFeatureCount(
    UPARAM(ref) const FCesiumFeatureIdSet& FeatureIDSet) {
  return FeatureIDSet._featureCount;
}

const int64 UCesiumFeatureIdSetBlueprintLibrary::GetNullFeatureID(
    UPARAM(ref) const FCesiumFeatureIdSet& FeatureIDSet) {
  return FeatureIDSet._nullFeatureID;
}

const FString UCesiumFeatureIdSetBlueprintLibrary::GetLabel(
    UPARAM(ref) const FCesiumFeatureIdSet& FeatureIDSet) {
  return FeatureIDSet._label;
}

int64 UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDForVertex(
    UPARAM(ref) const FCesiumFeatureIdSet& FeatureIDSet,
    int64 VertexIndex) {
  if (FeatureIDSet._featureIDSetType == ECesiumFeatureIdSetType::Attribute) {
    FCesiumFeatureIdAttribute attribute =
        std::get<FCesiumFeatureIdAttribute>(FeatureIDSet._featureID);
    return UCesiumFeatureIdAttributeBlueprintLibrary::GetFeatureIDForVertex(
        attribute,
        VertexIndex);
  }

  if (FeatureIDSet._featureIDSetType == ECesiumFeatureIdSetType::Texture) {
    FCesiumFeatureIdTexture texture =
        std::get<FCesiumFeatureIdTexture>(FeatureIDSet._featureID);
    return UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDForVertex(
        texture,
        VertexIndex);
  }

  if (FeatureIDSet._featureIDSetType == ECesiumFeatureIdSetType::Implicit) {
    return (VertexIndex >= 0 && VertexIndex < FeatureIDSet._featureCount)
               ? VertexIndex
               : -1;
  }

  return -1;
}

int64 UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDFromHit(
    UPARAM(ref) const FCesiumFeatureIdSet& FeatureIDSet,
    const FHitResult& Hit) {
  if (FeatureIDSet._featureIDSetType == ECesiumFeatureIdSetType::Texture) {
    FCesiumFeatureIdTexture texture =
        std::get<FCesiumFeatureIdTexture>(FeatureIDSet._featureID);
    return UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDFromHit(
        texture,
        Hit);
  }

  // Find the first vertex of the face.
  const UCesiumGltfPrimitiveComponent* pGltfComponent =
      Cast<UCesiumGltfPrimitiveComponent>(Hit.Component);
  if (!IsValid(pGltfComponent)) {
    return -1;
  }

  const CesiumPrimitiveData* pPrimData = pGltfComponent->getPrimitiveData();
  if (!pPrimData->pMeshPrimitive) {
    return -1;
  }
  auto VertexIndices = std::visit(
      CesiumGltf::IndicesForFaceFromAccessor{
          Hit.FaceIndex,
          pPrimData->PositionAccessor.size(),
          pPrimData->pMeshPrimitive->mode},
      pPrimData->IndexAccessor);

  int64 VertexIndex = VertexIndices[0];

  if (FeatureIDSet._featureIDSetType == ECesiumFeatureIdSetType::Attribute) {
    FCesiumFeatureIdAttribute attribute =
        std::get<FCesiumFeatureIdAttribute>(FeatureIDSet._featureID);
    return UCesiumFeatureIdAttributeBlueprintLibrary::GetFeatureIDForVertex(
        attribute,
        VertexIndex);
  }

  if (FeatureIDSet._featureIDSetType == ECesiumFeatureIdSetType::Implicit) {
    return (VertexIndex >= 0 && VertexIndex < FeatureIDSet._featureCount)
               ? VertexIndex
               : -1;
  }

  return -1;
}
