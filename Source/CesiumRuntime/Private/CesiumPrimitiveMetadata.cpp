// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumPrimitiveMetadata.h"
#include "CesiumGltf/AccessorView.h"
#include "CesiumGltf/ExtensionMeshPrimitiveExtStructuralMetadata.h"
#include "CesiumGltf/Model.h"
#include "CesiumGltfPrimitiveComponent.h"

static FCesiumPrimitiveMetadata EmptyPrimitiveMetadata;

FCesiumPrimitiveMetadata::FCesiumPrimitiveMetadata(
    const CesiumGltf::MeshPrimitive& Primitive,
    const CesiumGltf::ExtensionMeshPrimitiveExtStructuralMetadata& Metadata)
    : _propertyTextureIndices(), _propertyAttributeIndices() {
  this->_propertyTextureIndices.Reserve(Metadata.propertyTextures.size());
  for (const int64 propertyTextureIndex : Metadata.propertyTextures) {
    this->_propertyTextureIndices.Emplace(propertyTextureIndex);
  }

  this->_propertyAttributeIndices.Reserve(Metadata.propertyAttributes.size());
  for (const int64 propertyAttributeIndex : Metadata.propertyAttributes) {
    this->_propertyAttributeIndices.Emplace(propertyAttributeIndex);
  }
}

const FCesiumPrimitiveMetadata&
UCesiumPrimitiveMetadataBlueprintLibrary::GetPrimitiveMetadata(
    const UPrimitiveComponent* component) {
  const UCesiumGltfPrimitiveComponent* pGltfComponent =
      Cast<UCesiumGltfPrimitiveComponent>(component);
  if (!IsValid(pGltfComponent)) {
    return EmptyPrimitiveMetadata;
  }

  return getPrimitiveBase(pGltfComponent)->Metadata;
}

const TArray<int64>&
UCesiumPrimitiveMetadataBlueprintLibrary::GetPropertyTextureIndices(
    UPARAM(ref) const FCesiumPrimitiveMetadata& PrimitiveMetadata) {
  return PrimitiveMetadata._propertyTextureIndices;
}

const TArray<int64>&
UCesiumPrimitiveMetadataBlueprintLibrary::GetPropertyAttributeIndices(
    UPARAM(ref) const FCesiumPrimitiveMetadata& PrimitiveMetadata) {
  return PrimitiveMetadata._propertyAttributeIndices;
}
