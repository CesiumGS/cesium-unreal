// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumMetadataPrimitive.h"
#include "CesiumGltf/ExtensionMeshPrimitiveExtFeatureMetadata.h"
#include "CesiumGltf/Model.h"

// REMOVE AFTER DEPRECATION
#include "CesiumGltf/ExtensionModelExtFeatureMetadata.h"

FCesiumMetadataPrimitive::FCesiumMetadataPrimitive(
    const CesiumGltf::Model& model,
    const CesiumGltf::MeshPrimitive& primitive,
    const CesiumGltf::ExtensionMeshPrimitiveExtFeatureMetadata& metadata,
    const CesiumGltf::ExtensionModelExtFeatureMetadata& modelMetadata) {

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
       metadata.featureIdAttributes) {
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

      this->_featureIdAttributes.Add(FCesiumFeatureIdAttribute(
          model,
          *accessor,
          featureID->second,
          UTF8_TO_TCHAR(attribute.featureTable.c_str())));

      // REMOVE AFTER DEPRECATION
      PRAGMA_DISABLE_DEPRECATION_WARNINGS
      auto featureTableIt =
          modelMetadata.featureTables.find(attribute.featureTable);
      if (featureTableIt == modelMetadata.featureTables.end()) {
        continue;
      }
      this->_featureTables_deprecated.Add((FCesiumMetadataFeatureTable(
          model,
          *accessor,
          featureTableIt->second)));
      PRAGMA_ENABLE_DEPRECATION_WARNINGS
    }
  }

  for (const CesiumGltf::FeatureIDTexture& featureIdTexture :
       metadata.featureIdTextures) {
    this->_featureIdTextures.Add(
        FCesiumFeatureIdTexture(model, featureIdTexture));
  }

  this->_featureTextureNames.Reserve(metadata.featureTextures.size());
  for (const std::string& featureTextureName : metadata.featureTextures) {
    this->_featureTextureNames.Emplace(
        UTF8_TO_TCHAR(featureTextureName.c_str()));
  }
}

PRAGMA_DISABLE_DEPRECATION_WARNINGS
const TArray<FCesiumMetadataFeatureTable>&
UCesiumMetadataPrimitiveBlueprintLibrary::GetFeatureTables(
    const FCesiumMetadataPrimitive& MetadataPrimitive) {
  return MetadataPrimitive._featureTables_deprecated;
}
PRAGMA_ENABLE_DEPRECATION_WARNINGS

const TArray<FCesiumFeatureIdAttribute>&
UCesiumMetadataPrimitiveBlueprintLibrary::GetFeatureIdAttributes(
    UPARAM(ref) const FCesiumMetadataPrimitive& MetadataPrimitive) {
  return MetadataPrimitive._featureIdAttributes;
}

const TArray<FCesiumFeatureIdTexture>&
UCesiumMetadataPrimitiveBlueprintLibrary::GetFeatureIdTextures(
    UPARAM(ref) const FCesiumMetadataPrimitive& MetadataPrimitive) {
  return MetadataPrimitive._featureIdTextures;
}

const TArray<FString>&
UCesiumMetadataPrimitiveBlueprintLibrary::GetFeatureTextureNames(
    UPARAM(ref) const FCesiumMetadataPrimitive& MetadataPrimitive) {
  return MetadataPrimitive._featureTextureNames;
}

int64 UCesiumMetadataPrimitiveBlueprintLibrary::GetFirstVertexIDFromFaceID(
    UPARAM(ref) const FCesiumMetadataPrimitive& MetadataPrimitive,
    int64 faceID) {
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
      MetadataPrimitive._vertexIDAccessor);
}
