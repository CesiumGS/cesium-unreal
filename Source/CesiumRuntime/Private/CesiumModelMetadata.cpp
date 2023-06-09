// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumModelMetadata.h"
#include "CesiumGltf/ExtensionModelExtStructuralMetadata.h"
#include "CesiumGltf/Model.h"

using namespace CesiumGltf;

FCesiumModelMetadata::FCesiumModelMetadata(
    const Model& model,
    const ExtensionModelExtStructuralMetadata& metadata) {
  const size_t propertyTablesSize = metadata.propertyTables.size();
  this->_propertyTables.Reserve(propertyTablesSize);
  for (size_t i = 0; i < propertyTablesSize; i++) {
    this->_propertyTables.Emplace(
        FCesiumPropertyTable(model, metadata.propertyTables[i]));
  }

  this->_featureTextures.Reserve(metadata.featureTextures.size());
  for (const auto& featureTextureIt : metadata.featureTextures) {
    this->_featureTextures.Emplace(
        UTF8_TO_TCHAR(featureTextureIt.first.c_str()),
        FCesiumFeatureTexture(model, featureTextureIt.second));
  }
}

/*static*/
const TArray<FCesiumPropertyTable>&
UCesiumModelMetadataBlueprintLibrary::GetPropertyTables(
    UPARAM(ref) const FCesiumModelMetadata& ModelMetadata) {
  return ModelMetadata._propertyTables;
}

///*static*/
//const TMap<FString, FCesiumFeatureTexture>&
//UCesiumMetadataModelBlueprintLibrary::GetFeatureTextures(
//    UPARAM(ref) const FCesiumMetadataModel& MetadataModel) {
//  return MetadataModel._featureTextures;
//}
