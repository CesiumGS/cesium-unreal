// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumFeatureIdTexture.h"

#include "CesiumGltf/ExtensionExtMeshFeaturesFeatureIdTexture.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltfPrimitiveComponent.h"

#include <optional>

using namespace CesiumGltf;
using namespace CesiumGltf::MeshFeatures;

namespace {
struct TexCoordFromAccessor {
  std::optional<glm::vec2> operator()(std::monostate) { return std::nullopt; }

  std::optional<glm::vec2>
  operator()(const AccessorView<AccessorTypes::VEC2<float>>& value) {
    if (vertexIndex < 0 || vertexIndex >= value.size()) {
      return std::nullopt;
    }

    return glm::vec2(value[vertexIndex].value[0], value[vertexIndex].value[1]);
  }

  std::optional<glm::vec2>
  operator()(const AccessorView<AccessorTypes::VEC2<uint8_t>>& value) {
    if (vertexIndex < 0 || vertexIndex >= value.size()) {
      return std::nullopt;
    }

    float u = static_cast<float>(value[vertexIndex].value[0]);
    float v = static_cast<float>(value[vertexIndex].value[1]);

    u /= std::numeric_limits<uint8_t>::max();
    v /= std::numeric_limits<uint8_t>::max();

    return glm::vec2(u, v);
  }

  std::optional<glm::vec2>
  operator()(const AccessorView<AccessorTypes::VEC2<uint16_t>>& value) {
    if (vertexIndex < 0 || vertexIndex >= value.size()) {
      return std::nullopt;
    }

    float u = static_cast<float>(value[vertexIndex].value[0]);
    float v = static_cast<float>(value[vertexIndex].value[1]);

    u /= std::numeric_limits<uint16_t>::max();
    v /= std::numeric_limits<uint16_t>::max();

    return glm::vec2(u, v);
  }

  int64 vertexIndex;
};
} // namespace

FCesiumFeatureIdTexture::FCesiumFeatureIdTexture(
    const Model& InModel,
    const MeshPrimitive Primitive,
    const ExtensionExtMeshFeaturesFeatureIdTexture& FeatureIdTexture,
    const FString& PropertyTableName)
    : _status(ECesiumFeatureIdTextureStatus::ErrorInvalidTexture),
      _featureIdTextureView(InModel, FeatureIdTexture),
      _texCoordAccessor(),
      _textureCoordinateIndex(FeatureIdTexture.texCoord),
      _propertyTableName(PropertyTableName) {
  if (_featureIdTextureView.status() ==
      FeatureIdTextureViewStatus::ErrorInvalidChannels) {
    _status = ECesiumFeatureIdTextureStatus::ErrorInvalidChannelAccess;
    return;
  }

  if (_featureIdTextureView.status() != FeatureIdTextureViewStatus::Valid) {
    return;
  }

  const std::string texCoordName =
      "TEXCOORD_" + std::to_string(_textureCoordinateIndex);
  auto texCoord = Primitive.attributes.find(texCoordName);
  if (texCoord == Primitive.attributes.end()) {
    _status = ECesiumFeatureIdTextureStatus::ErrorInvalidTexCoord;
    return;
  }

  const Accessor* accessor =
      InModel.getSafe<Accessor>(&InModel.accessors, texCoord->second);
  if (!accessor || accessor->type != Accessor::Type::VEC2) {
    _status = ECesiumFeatureIdTextureStatus::ErrorInvalidTexCoordAccessor;
    return;
  }

  switch (accessor->componentType) {
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_BYTE:
    if (!accessor->normalized) {
      // Unsigned byte texcoords must be normalized.
      _status = ECesiumFeatureIdTextureStatus::ErrorInvalidTexCoordAccessor;
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
      _status = ECesiumFeatureIdTextureStatus::ErrorInvalidTexCoordAccessor;
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
    _status = ECesiumFeatureIdTextureStatus::ErrorInvalidTexCoordAccessor;
    return;
  }

  _status = ECesiumFeatureIdTextureStatus::Valid;
}

const FString& UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureTableName(
    UPARAM(ref) const FCesiumFeatureIdTexture& FeatureIDTexture) {
  return FeatureIDTexture._propertyTableName;
}

ECesiumFeatureIdTextureStatus
UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDTextureStatus(
    UPARAM(ref) const FCesiumFeatureIdTexture& FeatureIDTexture) {
  return FeatureIDTexture._status;
}

int64 UCesiumFeatureIdTextureBlueprintLibrary::GetTextureCoordinateIndex(
    const UPrimitiveComponent* component,
    const FCesiumFeatureIdTexture& FeatureIDTexture) {
  const UCesiumGltfPrimitiveComponent* pPrimitive =
      Cast<UCesiumGltfPrimitiveComponent>(component);
  if (!pPrimitive) {
    return -1;
  }

  if (FeatureIDTexture._status != ECesiumFeatureIdTextureStatus::Valid) {
    return -1;
  }

  auto textureCoordinateIndexIt = pPrimitive->textureCoordinateMap.find(
      FeatureIDTexture._featureIdTextureView.getTexCoordSetIndex());
  if (textureCoordinateIndexIt == pPrimitive->textureCoordinateMap.end()) {
    return -1;
  }

  return textureCoordinateIndexIt->second;
}

int64 UCesiumFeatureIdTextureBlueprintLibrary::
    GetFeatureIDForTextureCoordinates(
        const FCesiumFeatureIdTexture& FeatureIDTexture,
        float u,
        float v) {
  if (FeatureIDTexture._featureIdTextureView.status() !=
      FeatureIdTextureViewStatus::Valid) {
    return -1;
  }

  return FeatureIDTexture._featureIdTextureView.getFeatureId(u, v);
}

int64 UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDForVertex(
    UPARAM(ref) const FCesiumFeatureIdTexture& FeatureIDTexture,
    int64 VertexIndex) {
  const std::optional<glm::vec2> texCoords = std::visit(
      TexCoordFromAccessor{VertexIndex},
      FeatureIDTexture._texCoordAccessor);

  if (!texCoords) {
    return -1;
  }

  return GetFeatureIDForTextureCoordinates(
      FeatureIDTexture,
      (*texCoords)[0],
      (*texCoords)[1]);
}
