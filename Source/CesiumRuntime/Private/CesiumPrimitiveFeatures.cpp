// Copyright 2020-2024 CesiumGS, Inc. and Contributors

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
    : _vertexCount(0), _primitiveMode(Primitive.mode) {
  this->_indexAccessor = CesiumGltf::getIndexAccessorView(Model, Primitive);

  auto positionIt = Primitive.attributes.find("POSITION");
  if (positionIt != Primitive.attributes.end()) {
    const Accessor& positionAccessor =
        Model.getSafe(Model.accessors, positionIt->second);
    _vertexCount = positionAccessor.count;
  }

  for (const CesiumGltf::FeatureId& FeatureId : Features.featureIds) {
    this->_featureIdSets.Add(FCesiumFeatureIdSet(Model, Primitive, FeatureId));
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

  return getPrimitiveData(pGltfComponent)->Features;
}

const TArray<FCesiumFeatureIdSet>&
UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSets(
    UPARAM(ref) const FCesiumPrimitiveFeatures& PrimitiveFeatures) {
  return PrimitiveFeatures._featureIdSets;
}

const TArray<FCesiumFeatureIdSet>
UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSetsOfType(
    UPARAM(ref) const FCesiumPrimitiveFeatures& PrimitiveFeatures,
    ECesiumFeatureIdSetType Type) {
  TArray<FCesiumFeatureIdSet> featureIdSets;
  for (int32 i = 0; i < PrimitiveFeatures._featureIdSets.Num(); i++) {
    const FCesiumFeatureIdSet& featureIdSet =
        PrimitiveFeatures._featureIdSets[i];
    if (UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDSetType(
            featureIdSet) == Type) {
      featureIdSets.Add(featureIdSet);
    }
  }
  return featureIdSets;
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

  auto VertexIndices = std::visit(
      CesiumGltf::IndicesForFaceFromAccessor{
          FaceIndex,
          PrimitiveFeatures._vertexCount,
          PrimitiveFeatures._primitiveMode},
      PrimitiveFeatures._indexAccessor);

  return VertexIndices[0];
}

int64 UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDFromFace(
    UPARAM(ref) const FCesiumPrimitiveFeatures& PrimitiveFeatures,
    int64 FaceIndex,
    int64 FeatureIDSetIndex) {
  if (FeatureIDSetIndex < 0 ||
      FeatureIDSetIndex >= PrimitiveFeatures._featureIdSets.Num()) {
    return -1;
  }

  return UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDForVertex(
      PrimitiveFeatures._featureIdSets[FeatureIDSetIndex],
      UCesiumPrimitiveFeaturesBlueprintLibrary::GetFirstVertexFromFace(
          PrimitiveFeatures,
          FaceIndex));
}

int64 UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDFromHit(
    UPARAM(ref) const FCesiumPrimitiveFeatures& PrimitiveFeatures,
    const FHitResult& Hit,
    int64 FeatureIDSetIndex) {
  if (FeatureIDSetIndex < 0 ||
      FeatureIDSetIndex >= PrimitiveFeatures._featureIdSets.Num()) {
    return -1;
  }

  return UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDFromHit(
      PrimitiveFeatures._featureIdSets[FeatureIDSetIndex],
      Hit);
}
