// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumInstanceFeatures.h"
#include "CesiumGltf/ExtensionExtInstanceFeatures.h"
#include "CesiumGltf/ExtensionExtMeshGpuInstancing.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltfPrimitiveComponent.h"

using namespace CesiumGltf;

static FCesiumInstanceFeatures EmptyInstanceFeatures;

FCesiumInstanceFeatures::FCesiumInstanceFeatures(
    const Model& Model,
    const Node& Node)
    : _instanceCount(0) {
  if (Node.mesh < 0 || Node.mesh >= Model.meshes.size()) {
    return;
  }
  const auto* pInstancing = Node.getExtension<ExtensionExtMeshGpuInstancing>();
  if (!pInstancing) {
    return;
  }
  const auto* pInstanceFeatures =
      Node.getExtension<ExtensionExtInstanceFeatures>();
  if (!pInstanceFeatures) {
    return;
  }

  for (const auto& featureId : pInstanceFeatures->featureIds) {
    _featureIdSets.Emplace(Model, Node, featureId);
  }
}

const FCesiumInstanceFeatures&
UCesiumInstanceFeaturesBlueprintLibrary::GetInstanceFeatures(
    const UPrimitiveComponent* component) {
  const auto* pInstancedComponent =
      Cast<UCesiumGltfInstancedComponent>(component);
  if (!pInstancedComponent) {
    return EmptyInstanceFeatures;
  }
  if (auto pInstanceFeatures = pInstancedComponent->pInstanceFeatures) {
    return *pInstanceFeatures;
  }
  return EmptyInstanceFeatures;
}

const TArray<FCesiumFeatureIdSet>&
UCesiumInstanceFeaturesBlueprintLibrary::GetFeatureIDSets(
    const FCesiumInstanceFeatures& InstanceFeatures) {
  return InstanceFeatures._featureIdSets;
}

const TArray<FCesiumFeatureIdSet>
UCesiumInstanceFeaturesBlueprintLibrary::GetFeatureIDSetsOfType(
    const FCesiumInstanceFeatures& InstanceFeatures,
    ECesiumFeatureIdSetType Type) {
  TArray<FCesiumFeatureIdSet> result;
  for (int32 i = 0; i < InstanceFeatures._featureIdSets.Num(); i++) {
    const FCesiumFeatureIdSet& set = InstanceFeatures._featureIdSets[i];
    if (UCesiumFeatureIdSetBlueprintLibrary::GetFeatureIDSetType(set) == Type) {
      result.Add(set);
    }
  }
  return result;
}

int64 UCesiumInstanceFeaturesBlueprintLibrary::GetFeatureIDFromInstance(
    const FCesiumInstanceFeatures& InstanceFeatures,
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
