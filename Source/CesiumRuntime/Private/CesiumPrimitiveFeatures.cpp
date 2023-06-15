// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumPrimitiveFeatures.h"
#include "CesiumGltf/ExtensionExtMeshFeatures.h"
#include "CesiumGltf/Model.h"

using namespace CesiumGltf;

namespace {
struct VertexIndexFromAccessor {
  int64 operator()(std::monostate) {
    const int64 vertexIndex = faceIndex * 3;
    if (vertexIndex < vertexCount) {
      return vertexIndex;
    }
    return -1;
  }

  template <typename T> int64 operator()(const AccessorView<T>& value) {
    if (value.status() != AccessorViewStatus::Valid) {
      // Invalid accessor.
      return -1;
    }

    const int64 vertexIndex = faceIndex * 3;
    if (vertexIndex >= value.size()) {
      // Invalid face index.
      return -1;
    }

    return static_cast<int64>(value[vertexIndex].value[0]);
  }

  int64 faceIndex;
  int64 vertexCount;
};
} // namespace

FCesiumPrimitiveFeatures::FCesiumPrimitiveFeatures(
    const Model& InModel,
    const MeshPrimitive& Primitive,
    const ExtensionExtMeshFeatures& Features)
    : _vertexCount(0) {
  const Accessor& indicesAccessor =
      InModel.getSafe(InModel.accessors, Primitive.indices);
  switch (indicesAccessor.componentType) {
  case Accessor::ComponentType::UNSIGNED_BYTE:
    _vertexIDAccessor =
        AccessorView<AccessorTypes::SCALAR<uint8_t>>(InModel, indicesAccessor);
    break;
  case Accessor::ComponentType::UNSIGNED_SHORT:
    _vertexIDAccessor =
        AccessorView<AccessorTypes::SCALAR<uint16_t>>(InModel, indicesAccessor);
    break;
  case Accessor::ComponentType::UNSIGNED_INT:
    _vertexIDAccessor =
        AccessorView<AccessorTypes::SCALAR<uint32_t>>(InModel, indicesAccessor);
    break;
  default:
    break;
  }

  auto positionIt = Primitive.attributes.find("POSITION");
  if (positionIt != Primitive.attributes.end()) {
    const Accessor& positionAccessor =
        InModel.getSafe(InModel.accessors, positionIt->second);
    _vertexCount = positionAccessor.count;
  }

  for (const CesiumGltf::ExtensionExtMeshFeaturesFeatureId& FeatureId :
       Features.featureIds) {
    this->_featureIDSets.Add(
        FCesiumFeatureIdSet(InModel, Primitive, FeatureId));
  }
}

const TArray<FCesiumFeatureIdSet>&
UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSets(
    UPARAM(ref) const FCesiumPrimitiveFeatures& PrimitiveFeatures) {
  return PrimitiveFeatures._featureIDSets;
}

const TArray<FCesiumFeatureIdSet>
UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSetsOfType(
    UPARAM(ref) const FCesiumPrimitiveFeatures& PrimitiveFeatures,
    ECesiumFeatureIdSetType Type) {
  TArray<FCesiumFeatureIdSet> featureIDSets;
  for (int32 i = 0; i < PrimitiveFeatures._featureIDSets.Num(); i++) {
    const FCesiumFeatureIdSet& featureIDSet =
        PrimitiveFeatures._featureIDSets[i];
    if (UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDSetType(featureIDSet) ==
        Type) {
      featureIDSets.Add(featureIDSet);
    }
  }
  return featureIDSets;
}

int64 UCesiumPrimitiveFeaturesBlueprintLibrary::GetVertexCount(
    UPARAM(ref) const FCesiumPrimitiveFeatures& PrimitiveFeatures) {
  return PrimitiveFeatures._vertexCount;
}

int64 UCesiumPrimitiveFeaturesBlueprintLibrary::GetFirstVertexFromFace(
    UPARAM(ref) const FCesiumPrimitiveFeatures& PrimitiveFeatures,
    int64 FaceIndex) {
  if (FaceIndex < 0) {
    return -1;
  }

  return std::visit(
      VertexIndexFromAccessor{FaceIndex, PrimitiveFeatures._vertexCount},
      PrimitiveFeatures._vertexIDAccessor);
}

int64 UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDFromFace(
    UPARAM(ref) const FCesiumPrimitiveFeatures& PrimitiveFeatures,
    UPARAM(ref) const FCesiumFeatureIdSet& FeatureIDSet,
    int64 FaceIndex) {
  return UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDForVertex(
      FeatureIDSet,
      UCesiumPrimitiveFeaturesBlueprintLibrary::GetFirstVertexFromFace(
          PrimitiveFeatures,
          FaceIndex));
}
