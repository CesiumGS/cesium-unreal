// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumModelMetadata.h"
#include "CesiumGltf/ExtensionModelExtStructuralMetadata.h"
#include "CesiumGltf/Model.h"

using namespace CesiumGltf;

FCesiumModelMetadata::FCesiumModelMetadata(
    const Model& InModel,
    const ExtensionModelExtStructuralMetadata& Metadata) {
  this->_propertyTables.Reserve(Metadata.propertyTables.size());
  for (const auto& propertyTable : Metadata.propertyTables) {
    this->_propertyTables.Emplace(FCesiumPropertyTable(InModel, propertyTable));
  }

  this->_propertyTextures.Reserve(Metadata.propertyTextures.size());
  for (const auto& propertyTexture : Metadata.propertyTextures) {
    this->_propertyTextures.Emplace(
        FCesiumPropertyTexture(InModel, propertyTexture));
  }
}

/*static*/
const TArray<FCesiumPropertyTable>&
UCesiumModelMetadataBlueprintLibrary::GetPropertyTables(
    UPARAM(ref) const FCesiumModelMetadata& ModelMetadata) {
  return ModelMetadata._propertyTables;
}

/*static*/
const TArray<FCesiumPropertyTexture>&
UCesiumModelMetadataBlueprintLibrary::GetPropertyTextures(
    UPARAM(ref) const FCesiumModelMetadata& ModelMetadata) {
  return ModelMetadata._propertyTextures;
}
