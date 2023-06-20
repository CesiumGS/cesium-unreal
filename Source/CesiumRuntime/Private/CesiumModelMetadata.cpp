// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumModelMetadata.h"
#include "CesiumGltf/ExtensionModelExtStructuralMetadata.h"
#include "CesiumGltf/Model.h"

using namespace CesiumGltf;

static FCesiumPropertyTable EmptyPropertyTable;
static FCesiumPropertyTexture EmptyPropertyTexture;

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

/*static*/ const FCesiumPropertyTable&
UCesiumModelMetadataBlueprintLibrary::GetPropertyTable(
    UPARAM(ref) const FCesiumModelMetadata& ModelMetadata,
    const int64 Index) {
  if (Index < 0 || Index >= ModelMetadata._propertyTables.Num()) {
    return EmptyPropertyTable;
  }

  return ModelMetadata._propertyTables[Index];
}

/*static*/ const TArray<FCesiumPropertyTable>
UCesiumModelMetadataBlueprintLibrary::GetPropertyTablesAtIndices(
    UPARAM(ref) const FCesiumModelMetadata& ModelMetadata,
    const TArray<int64>& Indices) {
  TArray<FCesiumPropertyTable> result;
  for (int64 Index : Indices) {
    result.Add(GetPropertyTable(ModelMetadata, Index));
  }
  return result;
}

/*static*/
const TArray<FCesiumPropertyTexture>&
UCesiumModelMetadataBlueprintLibrary::GetPropertyTextures(
    UPARAM(ref) const FCesiumModelMetadata& ModelMetadata) {
  return ModelMetadata._propertyTextures;
}

/*static*/ const FCesiumPropertyTexture&
UCesiumModelMetadataBlueprintLibrary::GetPropertyTexture(
    UPARAM(ref) const FCesiumModelMetadata& ModelMetadata,
    const int64 Index) {
  if (Index < 0 || Index >= ModelMetadata._propertyTextures.Num()) {
    return EmptyPropertyTexture;
  }

  return ModelMetadata._propertyTextures[Index];
}

/*static*/ const TArray<FCesiumPropertyTexture>
UCesiumModelMetadataBlueprintLibrary::GetPropertyTexturesAtIndices(
    UPARAM(ref) const FCesiumModelMetadata& ModelMetadata,
    const TArray<int64>& Indices) {
  TArray<FCesiumPropertyTexture> result;
  for (int64 Index : Indices) {
    result.Add(GetPropertyTexture(ModelMetadata, Index));
  }
  return result;
}
