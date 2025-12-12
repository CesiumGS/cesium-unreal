// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumPrimitiveMetadata.h"
#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumPropertyAttribute.h"

#include <CesiumGltf/ExtensionMeshPrimitiveExtStructuralMetadata.h>
#include <CesiumGltf/ExtensionModelExtStructuralMetadata.h>
#include <CesiumGltf/Model.h>

static FCesiumPrimitiveMetadata EmptyPrimitiveMetadata;

FCesiumPrimitiveMetadata::FCesiumPrimitiveMetadata(
    const CesiumGltf::Model& model,
    const CesiumGltf::MeshPrimitive& primitive,
    const CesiumGltf::ExtensionMeshPrimitiveExtStructuralMetadata& metadata)
    : _propertyTextureIndices(),
      _propertyAttributes(),
      _propertyAttributeIndices() {
  this->_propertyTextureIndices.Reserve(metadata.propertyTextures.size());
  for (const int64 propertyTextureIndex : metadata.propertyTextures) {
    this->_propertyTextureIndices.Emplace(propertyTextureIndex);
  }

  // For backwards compatibility with GetPropertyAttributeIndices().
  this->_propertyAttributeIndices.Reserve(metadata.propertyAttributes.size());
  for (const int64 propertyAttributeIndex : metadata.propertyAttributes) {
    this->_propertyAttributeIndices.Emplace(propertyAttributeIndex);
  }

  const auto* pModelMetadata =
      model.getExtension<CesiumGltf::ExtensionModelExtStructuralMetadata>();
  if (!pModelMetadata) {
    return;
  }

  for (const int64 propertyAttributeIndex : metadata.propertyAttributes) {
    if (propertyAttributeIndex < 0 ||
        propertyAttributeIndex >=
            int64_t(pModelMetadata->propertyAttributes.size())) {
      continue;
    }

    this->_propertyAttributes.Emplace(FCesiumPropertyAttribute(
        model,
        primitive,
        pModelMetadata->propertyAttributes[propertyAttributeIndex]));
  }
}

const FCesiumPrimitiveMetadata&
UCesiumPrimitiveMetadataBlueprintLibrary::GetPrimitiveMetadata(
    const UPrimitiveComponent* component) {
  const UCesiumGltfInstancedComponent* pGltfInstancedComponent =
      Cast<UCesiumGltfInstancedComponent>(component);
  if (IsValid(pGltfInstancedComponent)) {
    return pGltfInstancedComponent->getPrimitiveData().Metadata;
  }

  const UCesiumGltfPrimitiveComponent* pGltfComponent =
      Cast<UCesiumGltfPrimitiveComponent>(component);
  if (IsValid(pGltfComponent)) {
    return pGltfComponent->getPrimitiveData().Metadata;
  }

  return EmptyPrimitiveMetadata;
}

const TArray<int64>&
UCesiumPrimitiveMetadataBlueprintLibrary::GetPropertyTextureIndices(
    UPARAM(ref) const FCesiumPrimitiveMetadata& PrimitiveMetadata) {
  return PrimitiveMetadata._propertyTextureIndices;
}

const TArray<FCesiumPropertyAttribute>&
UCesiumPrimitiveMetadataBlueprintLibrary::GetPropertyAttributes(
    UPARAM(ref) const FCesiumPrimitiveMetadata& PrimitiveMetadata) {
  return PrimitiveMetadata._propertyAttributes;
}

const TArray<int64>&
UCesiumPrimitiveMetadataBlueprintLibrary::GetPropertyAttributeIndices(
    UPARAM(ref) const FCesiumPrimitiveMetadata& PrimitiveMetadata) {
  return PrimitiveMetadata._propertyAttributeIndices;
}
