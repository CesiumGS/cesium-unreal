// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumGltfPrimitiveComponent.h"
#include "CalcBounds.h"
#include "CesiumLifetime.h"
#include "CesiumMaterialUserData.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PhysicsEngine/BodySetup.h"
#include "VecMath.h"

#include <CesiumGltf/MeshPrimitive.h>
#include <CesiumGltf/Model.h>
#include <variant>

// Prevent deprecation warnings while initializing deprecated metadata structs.
PRAGMA_DISABLE_DEPRECATION_WARNINGS

// Sets default values for this component's properties
UCesiumGltfPrimitiveComponent::UCesiumGltfPrimitiveComponent() {
  PrimaryComponentTick.bCanEverTick = false;
  pModel = nullptr;
  pMeshPrimitive = nullptr;
  pTilesetActor = nullptr;
}

PRAGMA_ENABLE_DEPRECATION_WARNINGS

UCesiumGltfPrimitiveComponent::~UCesiumGltfPrimitiveComponent() {}

void UCesiumGltfPrimitiveComponent::UpdateTransformFromCesium(
    const glm::dmat4& CesiumToUnrealTransform) {
  const FTransform transform = FTransform(VecMath::createMatrix(
      CesiumToUnrealTransform * this->HighPrecisionNodeTransform));

  if (this->Mobility == EComponentMobility::Movable) {
    // For movable objects, move the component in the normal way, but don't
    // generate collisions along the way. Teleporting physics is imperfect, but
    // it's the best available option.
    this->SetRelativeTransform(
        transform,
        false,
        nullptr,
        ETeleportType::TeleportPhysics);
  } else {
    // Unreall will yell at us for calling SetRelativeTransform on a static
    // object, but we still need to adjust (accurately!) for origin rebasing and
    // georeference changes. It's "ok" to move a static object in this way
    // because, we assume, the globe and globe-oriented lights, etc. are moving
    // too, so in a relative sense the object isn't actually moving. This isn't
    // a perfect assumption, of course.
    this->SetRelativeTransform_Direct(transform);
    this->UpdateComponentToWorld();
    this->MarkRenderTransformDirty();
    this->SendPhysicsTransform(ETeleportType::ResetPhysics);
  }
}

void UCesiumGltfPrimitiveComponent::BeginDestroy() {
  // Clear everything we can in order to reduce memory usage, because this
  // UObject might not actually get deleted by the garbage collector until much
  // later.
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

  UMaterialInstanceDynamic* pMaterial =
      Cast<UMaterialInstanceDynamic>(this->GetMaterial(0));
  if (pMaterial) {
    CesiumLifetime::destroy(pMaterial);
  }

  UStaticMesh* pMesh = this->GetStaticMesh();
  if (pMesh) {
    UBodySetup* pBodySetup = pMesh->GetBodySetup();

    if (pBodySetup) {
      CesiumLifetime::destroy(pBodySetup);
    }

    CesiumLifetime::destroy(pMesh);
  }

  Super::BeginDestroy();
}

FBoxSphereBounds UCesiumGltfPrimitiveComponent::CalcBounds(
    const FTransform& LocalToWorld) const {
  if (!this->boundingVolume) {
    return Super::CalcBounds(LocalToWorld);
  }

  return std::visit(
      CalcBoundsOperation{LocalToWorld, this->HighPrecisionNodeTransform},
      *this->boundingVolume);
}
