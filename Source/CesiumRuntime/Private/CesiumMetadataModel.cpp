// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumMetadataModel.h"
#include "CesiumGltf/ExtensionModelExtFeatureMetadata.h"
#include "CesiumGltf/Model.h"

using namespace CesiumGltf;

FCesiumMetadataModel::FCesiumMetadataModel(
    const Model& model,
    const ExtensionModelExtFeatureMetadata& metadata) {

  this->_featureTables.Reserve(metadata.featureTables.size());
  for (const auto& featureTableIt : metadata.featureTables) {
    this->_featureTables.Emplace(
        UTF8_TO_TCHAR(featureTableIt.first.c_str()),
        FCesiumFeatureTable(model, featureTableIt.second));
  }

  this->_featureTextures.Reserve(metadata.featureTextures.size());
  for (const auto& featureTextureIt : metadata.featureTextures) {
    this->_featureTextures.Emplace(
        UTF8_TO_TCHAR(featureTextureIt.first.c_str()),
        FCesiumFeatureTexture(model, featureTextureIt.second));
  }
}

/*static*/
const TMap<FString, FCesiumFeatureTable>&
UCesiumMetadataModelBlueprintLibrary::GetFeatureTables(
    UPARAM(ref) const FCesiumMetadataModel& MetadataModel) {
  return MetadataModel._featureTables;
}

/*static*/
const TMap<FString, FCesiumFeatureTexture>&
UCesiumMetadataModelBlueprintLibrary::GetFeatureTextures(
    UPARAM(ref) const FCesiumMetadataModel& MetadataModel) {
  return MetadataModel._featureTextures;
}
