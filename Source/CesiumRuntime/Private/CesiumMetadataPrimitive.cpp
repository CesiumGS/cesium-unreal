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

const TArray<FCesiumMetadataFeatureTable>&
UCesiumMetadataPrimitiveBlueprintLibrary::GetFeatureTables(
    const FCesiumMetadataPrimitive& metadataPrimitive) {
  return metadataPrimitive.GetFeatureTables();
}
