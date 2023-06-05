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
    this->_featureIDs.Add(FCesiumFeatureID(InModel, Primitive, FeatureId));
  }
}

const TArray<FCesiumFeatureID>&
UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDs(
    UPARAM(ref) const FCesiumPrimitiveFeatures& PrimitiveFeatures) {
  return PrimitiveFeatures._featureIDs;
}

const TArray<FCesiumFeatureID>
UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDsOfType(
    UPARAM(ref) const FCesiumPrimitiveFeatures& PrimitiveFeatures,
    ECesiumFeatureIDType Type) {
  TArray<FCesiumFeatureID> featureIDs;
  for (int32 i = 0; i < PrimitiveFeatures._featureIDs.Num(); i++) {
    const FCesiumFeatureID& featureID = PrimitiveFeatures._featureIDs[i];
    if (UCesiumFeatureIDBlueprintLibrary::GetFeatureIDType(featureID) == Type) {
      featureIDs.Add(featureID);
    }
  }
  return featureIDs;
}

int64 UCesiumPrimitiveFeaturesBlueprintLibrary::
    GetFirstVertexIndexFromFaceIndex(
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

int64 UCesiumPrimitiveFeaturesBlueprintLibrary::GetFeatureIDFromFaceIndex(
    UPARAM(ref) const FCesiumPrimitiveFeatures& PrimitiveFeatures,
    UPARAM(ref) const FCesiumFeatureID& FeatureID,
    int64 FaceIndex) {
  return UCesiumFeatureIDBlueprintLibrary::GetFeatureIDForVertex(
      FeatureID,
      UCesiumPrimitiveFeaturesBlueprintLibrary::
          GetFirstVertexIndexFromFaceIndex(PrimitiveFeatures, FaceIndex));
}
