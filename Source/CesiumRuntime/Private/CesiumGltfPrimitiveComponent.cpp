// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumGltfPrimitiveComponent.h"
#include "CalcBounds.h"
#include "CesiumLifetime.h"
#include "CesiumMaterialUserData.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PhysicsEngine/BodySetup.h"
#include "VecMath.h"

#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <variant>

// Prevent deprecation warnings while initializing deprecated metadata structs.
PRAGMA_DISABLE_DEPRECATION_WARNINGS

CesiumGltfPrimitiveBase::CesiumGltfPrimitiveBase() {
  pModel = nullptr;
  pMeshPrimitive = nullptr;
  pTilesetActor = nullptr;
}

// Sets default values for this component's properties
UCesiumGltfPrimitiveComponent::UCesiumGltfPrimitiveComponent() {
  PrimaryComponentTick.bCanEverTick = false;
}

UCesiumGltfInstancedComponent::UCesiumGltfInstancedComponent() {
  PrimaryComponentTick.bCanEverTick = false;
}

PRAGMA_ENABLE_DEPRECATION_WARNINGS

UCesiumGltfPrimitiveComponent::~UCesiumGltfPrimitiveComponent() {}

UCesiumGltfInstancedComponent::~UCesiumGltfInstancedComponent() {}

void CesiumGltfPrimitiveBase::BeginDestroyPrimitive() {
  this->Features = FCesiumPrimitiveFeatures();
  this->Metadata = FCesiumPrimitiveMetadata();
  this->EncodedFeatures =
      CesiumEncodedFeaturesMetadata::EncodedPrimitiveFeatures();
  this->EncodedMetadata =
      CesiumEncodedFeaturesMetadata::EncodedPrimitiveMetadata();

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  this->Metadata_DEPRECATED = FCesiumMetadataPrimitive();
  this->EncodedMetadata_DEPRECATED.reset();
  PRAGMA_ENABLE_DEPRECATION_WARNINGS

  this->pTilesetActor = nullptr;
  this->pModel = nullptr;
  this->pMeshPrimitive = nullptr;

  std::unordered_map<int32_t, uint32_t> emptyTexCoordMap;
  this->GltfToUnrealTexCoordMap.swap(emptyTexCoordMap);

  std::unordered_map<int32_t, CesiumGltf::TexCoordAccessorType>
      emptyAccessorMap;
  this->TexCoordAccessorMap.swap(emptyAccessorMap);
}

void UCesiumGltfPrimitiveComponent::BeginDestroy() {
  ::BeginDestroyPrimitive(this, getPrimitiveBase(this));
  Super::BeginDestroy();
}

void UCesiumGltfInstancedComponent::BeginDestroy() {
  ::BeginDestroyPrimitive(this, getPrimitiveBase(this));
  Super::BeginDestroy();
}

FBoxSphereBounds UCesiumGltfPrimitiveComponent::CalcBounds(
    const FTransform& LocalToWorld) const {
  if (!this->_cesiumBase.boundingVolume) {
    return Super::CalcBounds(LocalToWorld);
  }

  return std::visit(
      CalcBoundsOperation{
          LocalToWorld,
          this->_cesiumBase.HighPrecisionNodeTransform},
      *this->_cesiumBase.boundingVolume);
}

FBoxSphereBounds UCesiumGltfInstancedComponent::CalcBounds(
    const FTransform& LocalToWorld) const {
  if (!_cesiumBase.boundingVolume) {
    return Super::CalcBounds(LocalToWorld);
  }

  return std::visit(
      CalcBoundsOperation{
          LocalToWorld,
          this->_cesiumBase.HighPrecisionNodeTransform},
      *this->_cesiumBase.boundingVolume);
}

CesiumGltfPrimitiveBase* getPrimitiveBase(USceneComponent* pComponent) {
  if (auto* instancedComponent =
          Cast<UCesiumGltfInstancedComponent>(pComponent)) {
    return &instancedComponent->_cesiumBase;
  } else if (
      auto* meshComponent = Cast<UCesiumGltfPrimitiveComponent>(pComponent)) {
    return &meshComponent->_cesiumBase;
  }
  return nullptr;
}

const CesiumGltfPrimitiveBase*
getPrimitiveBase(const USceneComponent* pComponent) {
  if (const auto* instancedComponent =
          Cast<UCesiumGltfInstancedComponent>(pComponent)) {
    return &instancedComponent->_cesiumBase;
  } else if (
      const auto* meshComponent =
          Cast<UCesiumGltfPrimitiveComponent>(pComponent)) {
    return &meshComponent->_cesiumBase;
  }
  return nullptr;
}
