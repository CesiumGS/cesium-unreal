// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumMetadataPrimitive.h"
#include "CesiumGltf/ExtensionMeshPrimitiveExtFeatureMetadata.h"
#include "CesiumGltf/Model.h"

FCesiumMetadataPrimitive::FCesiumMetadataPrimitive(
    const CesiumGltf::Model& model,
    const CesiumGltf::MeshPrimitive& primitive,
    const CesiumGltf::ExtensionMeshPrimitiveExtFeatureMetadata& metadata) {

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

      this->_vertexFeatures.Add(FCesiumVertexMetadata(
          model,
          *accessor,
          featureID->second,
          UTF8_TO_TCHAR(attribute.featureTable.c_str())));
    }
  }

  for (const CesiumGltf::FeatureIDTexture& featureIDTexture :
       metadata.featureIdTextures) {
    this->_featureIDTextures.Add(
        FCesiumFeatureIDTexture(model, featureIDTexture));
  }

  this->_featureTextureNames.Reserve(metadata.featureTextures.size());
  for (const std::string& featureTextureName : metadata.featureTextures) {
    this->_featureTextureNames.Emplace(
        UTF8_TO_TCHAR(featureTextureName.c_str()));
  }
}

const TArray<FCesiumVertexMetadata>&
UCesiumMetadataPrimitiveBlueprintLibrary::GetVertexFeatures(
    UPARAM(ref) const FCesiumMetadataPrimitive& MetadataPrimitive) {
  return MetadataPrimitive._vertexFeatures;
}

const TArray<FCesiumFeatureIDTexture>&
UCesiumMetadataPrimitiveBlueprintLibrary::GetFeatureIDTextures(
    UPARAM(ref) const FCesiumMetadataPrimitive& MetadataPrimitive) {
  return MetadataPrimitive._featureIDTextures;
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
