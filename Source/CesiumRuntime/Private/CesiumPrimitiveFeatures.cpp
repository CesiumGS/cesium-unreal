// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumPrimitiveFeatures.h"
#include "CesiumGltf/ExtensionExtMeshFeatures.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltfPrimitiveComponent.h"

using namespace CesiumGltf;

static FCesiumPrimitiveFeatures EmptyPrimitiveFeatures;

FCesiumPrimitiveFeatures::FCesiumPrimitiveFeatures(
    const Model& Model,
    const MeshPrimitive& Primitive,
    const ExtensionExtMeshFeatures& Features)
    : _vertexCount(0) {
  const Accessor& indicesAccessor =
      Model.getSafe(Model.accessors, Primitive.indices);
  switch (indicesAccessor.componentType) {
  case Accessor::ComponentType::UNSIGNED_BYTE:
    _vertexIDAccessor = AccessorView<uint8_t>(Model, indicesAccessor);
    break;
  case Accessor::ComponentType::UNSIGNED_SHORT:
    _vertexIDAccessor = AccessorView<uint16_t>(Model, indicesAccessor);
    break;
  case Accessor::ComponentType::UNSIGNED_INT:
    _vertexIDAccessor = AccessorView<uint32_t>(Model, indicesAccessor);
    break;
  default:
    break;
  }

  auto positionIt = Primitive.attributes.find("POSITION");
  if (positionIt != Primitive.attributes.end()) {
    const Accessor& positionAccessor =
        Model.getSafe(Model.accessors, positionIt->second);
    _vertexCount = positionAccessor.count;
  }

  for (const CesiumGltf::FeatureId& FeatureId : Features.featureIds) {
    this->_featureIDSets.Add(FCesiumFeatureIdSet(Model, Primitive, FeatureId));
  }
}

const FCesiumPrimitiveFeatures&
UCesiumPrimitiveFeaturesBlueprintLibrary::GetPrimitiveFeatures(
    const UPrimitiveComponent* component) {
  const UCesiumGltfPrimitiveComponent* pGltfComponent =
      Cast<UCesiumGltfPrimitiveComponent>(component);
  if (!IsValid(pGltfComponent)) {
    return EmptyPrimitiveFeatures;
  }

  return pGltfComponent->Features;
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
    if (UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDSetType(
            featureIDSet) == Type) {
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

  std::array<int64, 3> faceIndices = std::visit(
      CesiumFaceVertexIndicesFromAccessor{
          FaceIndex,
          PrimitiveFeatures._vertexCount},
      PrimitiveFeatures._vertexIDAccessor);

  return faceIndices[0];
}

int64 UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDFromFace(
    UPARAM(ref) const FCesiumPrimitiveFeatures& PrimitiveFeatures,
    int64 FaceIndex,
    int64 FeatureIDSetIndex) {
  if (FeatureIDSetIndex < 0 ||
      FeatureIDSetIndex >= PrimitiveFeatures._featureIDSets.Num()) {
    return -1;
  }

  return UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDForVertex(
      PrimitiveFeatures._featureIDSets[FeatureIDSetIndex],
      UCesiumPrimitiveFeaturesBlueprintLibrary::GetFirstVertexFromFace(
          PrimitiveFeatures,
          FaceIndex));
}

int64 UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDFromHit(
    UPARAM(ref) const FCesiumPrimitiveFeatures& PrimitiveFeatures,
    const FHitResult& Hit,
    int64 FeatureIDSetIndex) {
  if (FeatureIDSetIndex < 0 ||
      FeatureIDSetIndex >= PrimitiveFeatures._featureIDSets.Num()) {
    return -1;
  }

  return UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDFromHit(
      PrimitiveFeatures._featureIDSets[FeatureIDSetIndex],
      Hit);
}
