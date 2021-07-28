// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumMetadataPrimitive.h"
#include "CesiumGltf/MeshPrimitiveEXT_feature_metadata.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltf/ModelEXT_feature_metadata.h"

FCesiumMetadataPrimitive::FCesiumMetadataPrimitive(
    const CesiumGltf::Model& model,
    const CesiumGltf::MeshPrimitive& primitive,
    const CesiumGltf::ModelEXT_feature_metadata& metadata,
    const CesiumGltf::MeshPrimitiveEXT_feature_metadata& primitiveMetadata) {

  const CesiumGltf::Accessor& indicesAccessor =
      model.getSafe(model.accessors, primitive.indices);
  switch (indicesAccessor.componentType) {
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE:
    _vertexIDAccessor =
        CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<uint8_t>>(
            model,
            indicesAccessor);
    break;
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT:
    _vertexIDAccessor =
        CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<uint16_t>>(
            model,
            indicesAccessor);
    break;
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_INT:
    _vertexIDAccessor =
        CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::SCALAR<uint32_t>>(
            model,
            indicesAccessor);
    break;
  default:
    break;
  }

  for (const CesiumGltf::FeatureIDAttribute& attribute :
       primitiveMetadata.featureIdAttributes) {
    if (attribute.featureIds.attribute) {
      auto featureID =
          primitive.attributes.find(attribute.featureIds.attribute.value());
      if (featureID == primitive.attributes.end()) {
        continue;
      }

      const CesiumGltf::Accessor* accessor =
          model.getSafe<CesiumGltf::Accessor>(
              &model.accessors,
              featureID->second);
      if (!accessor) {
        continue;
      }

      if (accessor->type != CesiumGltf::Accessor::Type::SCALAR) {
        continue;
      }

      auto featureTable = metadata.featureTables.find(attribute.featureTable);
      if (featureTable == metadata.featureTables.end()) {
        continue;
      }

      _featureTables.Add((
          FCesiumMetadataFeatureTable(model, *accessor, featureTable->second)));
    }
  }
}

const TArray<FCesiumMetadataFeatureTable>&
FCesiumMetadataPrimitive::GetFeatureTables() const {
  return _featureTables;
}

int64 FCesiumMetadataPrimitive::GetFirstVertexIDFromFaceID(int64 faceID) const {
  return std::visit(
      [faceID](const auto& accessor) {
        int64 index = faceID * 3;

        if (accessor.status() != CesiumGltf::AccessorViewStatus::Valid) {
          // No indices, so each successive face is just the next three
          // vertices.
          return index;
        } else if (index >= accessor.size()) {
          // Invalid face index.
          return -1LL;
        }

        return int64(accessor[index].value[0]);
      },
      _vertexIDAccessor);
}

const TArray<FCesiumMetadataFeatureTable>&
UCesiumMetadataPrimitiveBlueprintLibrary::GetFeatureTables(
    const FCesiumMetadataPrimitive& metadataPrimitive) {
  return metadataPrimitive.GetFeatureTables();
}

int64 UCesiumMetadataPrimitiveBlueprintLibrary::GetFirstVertexIDFromFaceID(
    UPARAM(ref) const FCesiumMetadataPrimitive& metadataPrimitive,
    int64 faceID) {
  return metadataPrimitive.GetFirstVertexIDFromFaceID(faceID);
}
