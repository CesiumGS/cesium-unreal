// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumFeatureIdTexture.h"
#include "CesiumGltf/FeatureIdTexture.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumMetadataPickingBlueprintLibrary.h"

#include <optional>

using namespace CesiumGltf;

FCesiumFeatureIdTexture::FCesiumFeatureIdTexture(
    const Model& Model,
    const MeshPrimitive& Primitive,
    const FeatureIdTexture& FeatureIdTexture,
    const FString& PropertyTableName)
    : _status(ECesiumFeatureIdTextureStatus::ErrorInvalidTexture),
      _featureIdTextureView(),
      _texCoordAccessor(),
      _textureCoordinateSetIndex(FeatureIdTexture.texCoord),
      _propertyTableName(PropertyTableName) {
  TextureViewOptions options;
  options.applyKhrTextureTransformExtension = true;

  if (FeatureIdTexture.extras.find("makeImageCopy") !=
      FeatureIdTexture.extras.end()) {
    options.makeImageCopy =
        FeatureIdTexture.extras.at("makeImageCopy").getBoolOrDefault(false);
  }

  this->_featureIdTextureView =
      FeatureIdTextureView(Model, FeatureIdTexture, options);

  switch (_featureIdTextureView.status()) {
  case FeatureIdTextureViewStatus::Valid:
    this->_status = ECesiumFeatureIdTextureStatus::Valid;
    break;
  case FeatureIdTextureViewStatus::ErrorInvalidChannels:
    this->_status = ECesiumFeatureIdTextureStatus::ErrorInvalidTextureAccess;
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
  this->_texCoordAccessor = CesiumGltf::getTexCoordAccessorView(
      Model,
      Primitive,
      this->_textureCoordinateSetIndex);
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

int64 UCesiumFeatureIdTextureBlueprintLibrary::GetGltfTextureCoordinateSetIndex(
    UPARAM(ref) const FCesiumFeatureIdTexture& FeatureIDTexture) {
  return FeatureIDTexture._featureIdTextureView.getTexCoordSetIndex();
}

int64 UCesiumFeatureIdTextureBlueprintLibrary::GetUnrealUVChannel(
    const UPrimitiveComponent* PrimitiveComponent,
    UPARAM(ref) const FCesiumFeatureIdTexture& FeatureIDTexture) {
  const UCesiumGltfPrimitiveComponent* pPrimitive =
      Cast<UCesiumGltfPrimitiveComponent>(PrimitiveComponent);
  if (!pPrimitive ||
      FeatureIDTexture._status != ECesiumFeatureIdTextureStatus::Valid) {
    return -1;
  }

  auto textureCoordinateIndexIt = pPrimitive->GltfToUnrealTexCoordMap.find(
      UCesiumFeatureIdTextureBlueprintLibrary::GetGltfTextureCoordinateSetIndex(
          FeatureIDTexture));
  if (textureCoordinateIndexIt == pPrimitive->GltfToUnrealTexCoordMap.end()) {
    return -1;
  }

  return textureCoordinateIndexIt->second;
}

PRAGMA_DISABLE_DEPRECATION_WARNINGS
int64 UCesiumFeatureIdTextureBlueprintLibrary::
    GetFeatureIDForTextureCoordinates(
        UPARAM(ref) const FCesiumFeatureIdTexture& FeatureIDTexture,
        float U,
        float V) {
  return FeatureIDTexture._featureIdTextureView.getFeatureID(U, V);
}
PRAGMA_ENABLE_DEPRECATION_WARNINGS

int64 UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDForUV(
    UPARAM(ref) const FCesiumFeatureIdTexture& FeatureIDTexture,
    const FVector2D& UV) {
  return FeatureIDTexture._featureIdTextureView.getFeatureID(UV[0], UV[1]);
}

int64 UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDForVertex(
    UPARAM(ref) const FCesiumFeatureIdTexture& FeatureIDTexture,
    int64 VertexIndex) {
  const std::optional<glm::dvec2> texCoords = std::visit(
      CesiumGltf::TexCoordFromAccessor{VertexIndex},
      FeatureIDTexture._texCoordAccessor);
  if (!texCoords) {
    return -1;
  }

  return FeatureIDTexture._featureIdTextureView.getFeatureID(
      (*texCoords)[0],
      (*texCoords)[1]);
}

int64 UCesiumFeatureIdTextureBlueprintLibrary::GetFeatureIDFromHit(
    UPARAM(ref) const FCesiumFeatureIdTexture& FeatureIDTexture,
    const FHitResult& Hit) {
  FVector2D UV;
  if (UCesiumMetadataPickingBlueprintLibrary::FindUVFromHit(
          Hit,
          FeatureIDTexture._featureIdTextureView.getTexCoordSetIndex(),
          UV)) {
    return FeatureIDTexture._featureIdTextureView.getFeatureID(UV[0], UV[1]);
  }

  return -1;
}
