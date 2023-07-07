// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumMetadataModel.h"

using namespace CesiumGltf;

FCesiumMetadataModel::FCesiumMetadataModel(
    const FCesiumModelMetadata& ModelMetadata)
    : _pModelMetadata(&ModelMetadata) {}

/*static*/
const TMap<FString, FCesiumPropertyTable&>
UCesiumMetadataModelBlueprintLibrary::GetFeatureTables(
    UPARAM(ref) const FCesiumMetadataModel& MetadataModel) {
  TMap<FString, FCesiumPropertyTable&> result;
  if (!MetadataModel._pModelMetadata) {
    return result;
  }

  const TArray<FCesiumPropertyTable>& propertyTables =
      UCesiumModelMetadataBlueprintLibrary::GetPropertyTables(
          *MetadataModel._pModelMetadata);
  result.Reserve(propertyTables.Num());

  for (const FCesiumPropertyTable& propertyTable : propertyTables) {
    const FString& name =
        UCesiumPropertyTableBlueprintLibrary::GetPropertyTableName(
            propertyTable);
    result.Emplace(name, propertyTable);
  }

  return result;
}

/*static*/
const TMap<FString, FCesiumPropertyTexture&>
UCesiumMetadataModelBlueprintLibrary::GetFeatureTextures(
    UPARAM(ref) const FCesiumMetadataModel& MetadataModel) {
  TMap<FString, FCesiumPropertyTexture&> result;
  if (!MetadataModel._pModelMetadata) {
    return result;
  }

  const TArray<FCesiumPropertyTexture>& propertyTextures =
      UCesiumModelMetadataBlueprintLibrary::GetPropertyTextures(
          *MetadataModel._pModelMetadata);
  result.Reserve(propertyTextures.Num());

  for (const FCesiumPropertyTexture& propertyTexture : propertyTextures) {
    const FString& name =
        UCesiumPropertyTextureBlueprintLibrary::GetPropertyTextureName(
            propertyTexture);
    result.Emplace(name, propertyTexture);
  }

  return result;
}
