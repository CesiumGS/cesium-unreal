// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumFeatureIdTexture.h"

#include "CesiumGltf/FeatureIdTexture.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltfPrimitiveComponent.h"

#include <optional>

using namespace CesiumGltf;

namespace {
// There are technically no invalid texcoord values because of clamp / wrap
// behavior, so we use std::nullopt to denote an erroneous value.
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
    const Model& Model,
    const MeshPrimitive& Primitive,
    const FeatureIdTexture& FeatureIdTexture,
    const FString& PropertyTableName)
    : _status(ECesiumFeatureIdTextureStatus::ErrorInvalidTexture),
      _featureIdTextureView(Model, FeatureIdTexture),
      _texCoordAccessor(),
      _textureCoordinateIndex(FeatureIdTexture.texCoord),
      _propertyTableName(PropertyTableName) {
  switch (_featureIdTextureView.status()) {
  case FeatureIdTextureViewStatus::Valid:
    _status = ECesiumFeatureIdTextureStatus::Valid;
    break;
  case FeatureIdTextureViewStatus::ErrorInvalidChannels:
    _status = ECesiumFeatureIdTextureStatus::ErrorInvalidTextureAccess;
    return;
  default:
    // Error with the texture or image. The status is already set by the
    // initializer list.
    return;
  }

  // The EXT_feature_metadata version of FCesiumFeatureIdTexture was not
  // constructed with an "owner" primitive. It was possible to access the
  // texture data with technically arbitrary coordinates.
  //
  // To maintain this functionality in EXT_mesh_features, the texture view will
  // still be valid if the intended texcoords don't exist. However, feature IDs
  // won't be retrievable by vertex index.
  const std::string texCoordName =
      "TEXCOORD_" + std::to_string(_textureCoordinateIndex);
  auto texCoord = Primitive.attributes.find(texCoordName);
  if (texCoord == Primitive.attributes.end()) {
    return;
  }

  const Accessor* accessor =
      Model.getSafe<Accessor>(&Model.accessors, texCoord->second);
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
            Model,
            *accessor);
    break;
  case CesiumGltf::Accessor::ComponentType::UNSIGNED_SHORT:
    if (!accessor->normalized) {
      // Unsigned short texcoords must be normalized.
      return;
    }
    this->_texCoordAccessor =
        CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::VEC2<uint16_t>>(
            Model,
            *accessor);
    break;
  case CesiumGltf::Accessor::ComponentType::FLOAT:
    this->_texCoordAccessor =
        CesiumGltf::AccessorView<CesiumGltf::AccessorTypes::VEC2<float>>(
            Model,
            *accessor);
    break;
  default:
    break;
  }
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
    const UPrimitiveComponent* PrimitiveComponent,
    const FCesiumFeatureIdTexture& FeatureIDTexture) {
  const UCesiumGltfPrimitiveComponent* pPrimitive =
      Cast<UCesiumGltfPrimitiveComponent>(PrimitiveComponent);
  if (!pPrimitive ||
      FeatureIDTexture._status != ECesiumFeatureIdTextureStatus::Valid) {
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
        float U,
        float V) {
  return FeatureIDTexture._featureIdTextureView.getFeatureID(U, V);
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
