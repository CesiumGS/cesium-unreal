// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumPrimitiveFeatures.h"
#include "CesiumGltf/ExtensionExtInstanceFeatures.h"
#include "CesiumGltf/ExtensionExtMeshFeatures.h"
#include "CesiumGltf/ExtensionExtMeshGpuInstancing.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltfPrimitiveComponent.h"

static FCesiumPrimitiveFeatures EmptyPrimitiveFeatures;

FCesiumPrimitiveFeatures::FCesiumPrimitiveFeatures(
    const CesiumGltf::Model& Model,
    const CesiumGltf::MeshPrimitive& Primitive,
    const CesiumGltf::ExtensionExtMeshFeatures& Features)
    : _vertexCount(0), _primitiveMode(Primitive.mode) {
  this->_indexAccessor = CesiumGltf::getIndexAccessorView(Model, Primitive);

  auto positionIt = Primitive.attributes.find("POSITION");
  if (positionIt != Primitive.attributes.end()) {
    const CesiumGltf::Accessor& positionAccessor =
        Model.getSafe(Model.accessors, positionIt->second);
    _vertexCount = positionAccessor.count;
  }

  for (const CesiumGltf::FeatureId& FeatureId : Features.featureIds) {
    this->_featureIdSets.Add(FCesiumFeatureIdSet(Model, Primitive, FeatureId));
  }
}

FCesiumPrimitiveFeatures::FCesiumPrimitiveFeatures(
    const CesiumGltf::Model& Model,
    const CesiumGltf::Node& Node,
    const CesiumGltf::ExtensionExtInstanceFeatures& InstanceFeatures)
    : _vertexCount(0), _primitiveMode(-1) {
  if (Node.mesh < 0 || Node.mesh >= Model.meshes.size()) {
    return;
  }
  for (const auto& featureId : InstanceFeatures.featureIds) {
    this->_featureIdSets.Emplace(Model, Node, featureId);
  }
}

const FCesiumPrimitiveFeatures&
UCesiumPrimitiveFeaturesBlueprintLibrary::GetPrimitiveFeatures(
    const UPrimitiveComponent* component) {
  const UCesiumGltfInstancedComponent* pGltfInstancedComponent =
      Cast<UCesiumGltfInstancedComponent>(component);
  if (IsValid(pGltfInstancedComponent)) {
    return *pGltfInstancedComponent->pInstanceFeatures;
  }

  const UCesiumGltfPrimitiveComponent* pGltfComponent =
      Cast<UCesiumGltfPrimitiveComponent>(component);
  if (IsValid(pGltfComponent)) {
    return pGltfComponent->getPrimitiveData().Features;
  }

  return EmptyPrimitiveFeatures;
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

int64 UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDFromInstance(
    const FCesiumPrimitiveFeatures& InstanceFeatures,
    int64 InstanceIndex,
    int64 FeatureIDSetIndex) {
  if (FeatureIDSetIndex < 0 ||
      FeatureIDSetIndex >= InstanceFeatures._featureIdSets.Num()) {
    return -1;
  }
  const auto& featureIDSet = InstanceFeatures._featureIdSets[FeatureIDSetIndex];
  return UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDForInstance(
      featureIDSet,
      InstanceIndex);
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
