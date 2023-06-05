// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumFeatureIdTexture.h"

#include "CesiumGltf/ExtensionExtMeshFeaturesFeatureIdTexture.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltfPrimitiveComponent.h"

using namespace CesiumGltf;
using namespace CesiumGltf::MeshFeatures;

struct TexCoordFromAccessor {
  glm::vec2 operator()(std::monostate) { return glm::vec2(-1, -1); }

  glm::vec2 operator()(const AccessorView<AccessorTypes::VEC2<float>>& value) {
    return glm::vec2(value[vertexIdx].value[0], value[vertexIdx].value[1]);
  }

  glm::vec2
  operator()(const AccessorView<AccessorTypes::VEC2<uint8_t>>& value) {
    float u = static_cast<float>(value[vertexIdx].value[0]);
    float v = static_cast<float>(value[vertexIdx].value[1]);

    u /= std::numeric_limits<uint8_t>::max();
    v /= std::numeric_limits<uint8_t>::max();

    return glm::vec2(u, v);
  }

  glm::vec2
  operator()(const AccessorView<AccessorTypes::VEC2<uint16_t>>& value) {
    float u = static_cast<float>(value[vertexIdx].value[0]);
    float v = static_cast<float>(value[vertexIdx].value[1]);

    u /= std::numeric_limits<uint16_t>::max();
    v /= std::numeric_limits<uint16_t>::max();

    return glm::vec2(u, v);
  }

  int64 vertexIdx;
};

FCesiumFeatureIDTexture::FCesiumFeatureIDTexture(
    const Model& InModel,
    const CesiumGltf::MeshPrimitive Primitive,
    const ExtensionExtMeshFeaturesFeatureIdTexture& FeatureIdTexture)
    : _featureIDTextureView(InModel, FeatureIdTexture),
      _texCoordAccessor(),
      _textureCoordinateIndex(-1) {
  const int64 texCoordIndex = _featureIDTextureView.getTexCoordSetIndex();
  const std::string texCoordName = "TEXCOORD_" + std::to_string(texCoordIndex);
  auto texCoord = Primitive.attributes.find(texCoordName);
  if (texCoord == Primitive.attributes.end()) {
    return;
  }

  const Accessor* accessor =
      InModel.getSafe<Accessor>(&InModel.accessors, texCoord->second);
  if (!accessor || accessor->type != Accessor::Type::VEC2) {
    return;
  }

  switch (accessor->componentType) {
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE:
    if (!accessor->normalized) {
      // Unsigned byte texcoords must be normalized.
      return;
    }

    this->_texCoordAccessor =
        CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::VEC2<uint8_t>>(
            InModel,
            *accessor);
    break;
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT:
    if (!accessor->normalized) {
      // Unsigned short texcoords must be normalized.
      return;
    }
    this->_texCoordAccessor =
        CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::VEC2<uint16_t>>(
            InModel,
            *accessor);
    break;
  case CesiumGltf::Accessor::ComponentType::FLOAT:
    this->_texCoordAccessor =
        CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::VEC2<float>>(
            InModel,
            *accessor);
    break;
  default:
    return;
  }

  _textureCoordinateIndex = texCoordIndex;
}

int64 UCesiumFeatureIDTextureBlueprintLibrary::GetTextureCoordinateIndex(
    const UPrimitiveComponent* component,
    const FCesiumFeatureIDTexture& featureIDTexture) {
  // TODO: where is this function used?
  const UCesiumGltfPrimitiveComponent* pPrimitive =
      Cast<UCesiumGltfPrimitiveComponent>(component);
  if (!pPrimitive) {
    return -1;
  }

  if (featureIDTexture._featureIDTextureView.status() !=
      FeatureIdTextureViewStatus::Valid) {
    return -1;
  }

  auto textureCoordinateIndexIt = pPrimitive->textureCoordinateMap.find(
      featureIDTexture._featureIDTextureView.getTexCoordSetIndex());
  if (textureCoordinateIndexIt == pPrimitive->textureCoordinateMap.end()) {
    return -1;
  }

  return textureCoordinateIndexIt->second;
}

int64 UCesiumFeatureIDTextureBlueprintLibrary::
    GetFeatureIDForTextureCoordinates(
        const FCesiumFeatureIDTexture& FeatureIDTexture,
        float u,
        float v) {
  if (FeatureIDTexture._featureIDTextureView.status() !=
      FeatureIdTextureViewStatus::Valid) {
    return -1;
  }

  return FeatureIDTexture._featureIDTextureView.getFeatureId(u, v);
}

int64 UCesiumFeatureIDTextureBlueprintLibrary::GetFeatureIDForVertex(
    UPARAM(ref) const FCesiumFeatureIDTexture& FeatureIDTexture,
    int64 VertexIndex) {
  const glm::vec2 texCoords = std::visit(
      TexCoordFromAccessor{VertexIndex},
      FeatureIDTexture._texCoordAccessor);

  return GetFeatureIDForTextureCoordinates(
      FeatureIDTexture,
      texCoords[0],
      texCoords[1]);
}
