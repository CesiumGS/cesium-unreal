// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumPrimitiveFeatures.h"
#include "CesiumGltf/ExtensionExtMeshFeatures.h"
#include "CesiumGltf/Model.h"

using namespace CesiumGltf;

FCesiumPrimitiveFeatures::FCesiumPrimitiveFeatures(
    const Model& InModel,
    const MeshPrimitive& Primitive,
    const ExtensionExtMeshFeatures& Features) {
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

  for (const CesiumGltf::ExtensionExtMeshFeaturesFeatureId& FeatureId :
       Features.featureIds) {
    this->_featureIDSets.Add(FCesiumFeatureIDSet(InModel, Primitive, FeatureId));
  }
}

const TArray<FCesiumFeatureIDSet>&
UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSets(
    UPARAM(ref) const FCesiumPrimitiveFeatures& PrimitiveFeatures) {
  return PrimitiveFeatures._featureIDSets;
}

const TArray<FCesiumFeatureIDSet>
UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDSetsOfType(
    UPARAM(ref) const FCesiumPrimitiveFeatures& PrimitiveFeatures,
    ECesiumFeatureIDType Type) {
  TArray<FCesiumFeatureIDSet> featureIDSets;
  for (int32 i = 0; i < PrimitiveFeatures._featureIDSets.Num(); i++) {
    const FCesiumFeatureIDSet& featureIDSet = PrimitiveFeatures._featureIDSets[i];
    if (UCesiumFeatureIDSetBlueprintLibrary::GetFeatureIDType(featureIDSet) == Type) {
      featureIDSets.Add(featureIDSet);
    }
  }
  return featureIDSets;
}

int64 UCesiumPrimitiveFeaturesBlueprintLibrary::GetFirstVertexIndexFromFace(
    UPARAM(ref) const FCesiumPrimitiveFeatures& PrimitiveFeatures,
    int64 FaceIndex) {
  return std::visit(
      [FaceIndex](const auto& accessor) {
        int64 index = FaceIndex * 3;

        if (accessor.status() != AccessorViewStatus::Valid) {
          // No indices, so each successive face is just the next three
          // vertices.
          return index;
        } else if (index >= accessor.size()) {
          // Invalid face index.
          return -1LL;
        }

        return int64(accessor[index].value[0]);
      },
      PrimitiveFeatures._vertexIDAccessor);
}

int64 UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDFromFace(
    UPARAM(ref) const FCesiumPrimitiveFeatures& PrimitiveFeatures,
    UPARAM(ref) const FCesiumFeatureIDSet& FeatureIDSet,
    int64 FaceIndex) {
  return UCesiumFeatureIDSetBlueprintLibrary::GetFeatureIDForVertex(
      FeatureIDSet,
      UCesiumPrimitiveFeaturesBlueprintLibrary::GetFirstVertexIndexFromFace(
          PrimitiveFeatures,
          FaceIndex));
}
