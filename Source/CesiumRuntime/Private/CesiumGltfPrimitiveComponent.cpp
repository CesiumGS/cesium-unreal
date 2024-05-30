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

namespace {
void destroyCesiumPrimitive(UStaticMeshComponent* uobject) {
  // Clear everything we can in order to reduce memory usage, because this
  // UObject might not actually get deleted by the garbage collector until
  // much later.
  auto* cesiumPrimitive = Cast<ICesiumPrimitive>(uobject);
  cesiumPrimitive->getPrimitiveData().destroy();
  UMaterialInstanceDynamic* pMaterial =
      Cast<UMaterialInstanceDynamic>(uobject->GetMaterial(0));
  if (pMaterial) {
    CesiumLifetime::destroy(pMaterial);
  }

  UStaticMesh* pMesh = uobject->GetStaticMesh();
  if (pMesh) {
    UBodySetup* pBodySetup = pMesh->GetBodySetup();

    if (pBodySetup) {
      CesiumLifetime::destroy(pBodySetup);
    }

    CesiumLifetime::destroy(pMesh);
  }
}
}
void UCesiumGltfPrimitiveComponent::BeginDestroy() {
  destroyCesiumPrimitive(this);
  Super::BeginDestroy();
}

void UCesiumGltfInstancedComponent::BeginDestroy() {
  destroyCesiumPrimitive(this);
  Super::BeginDestroy();
}

FBoxSphereBounds UCesiumGltfPrimitiveComponent::CalcBounds(
    const FTransform& LocalToWorld) const {
  if (!getPrimitiveData().boundingVolume) {
    return Super::CalcBounds(LocalToWorld);
  }

  return std::visit(
      CalcBoundsOperation{
          LocalToWorld,
          getPrimitiveData().HighPrecisionNodeTransform},
      *getPrimitiveData().boundingVolume);
}

FBoxSphereBounds UCesiumGltfInstancedComponent::CalcBounds(
    const FTransform& LocalToWorld) const {
  if (!getPrimitiveData().boundingVolume) {
    return Super::CalcBounds(LocalToWorld);
  }

  return std::visit(
      CalcBoundsOperation{
          LocalToWorld,
          getPrimitiveData().HighPrecisionNodeTransform},
      *getPrimitiveData().boundingVolume);
}

void UCesiumGltfPrimitiveComponent::resetPhysicsTransform() {
  SendPhysicsTransform(ETeleportType::ResetPhysics);
}

void UCesiumGltfInstancedComponent::resetPhysicsTransform() {
  SendPhysicsTransform(ETeleportType::ResetPhysics);
}

void UpdateTransformFromCesium(
    const glm::dmat4& CesiumToUnrealTransform,
    UStaticMeshComponent* uobject) {
  auto* pCesiumPrimitive = Cast<ICesiumPrimitive>(uobject);
  if (!pCesiumPrimitive) {
    return;
  }
  const CesiumPrimitiveData& primData = pCesiumPrimitive->getPrimitiveData();
  const FTransform transform = FTransform(VecMath::createMatrix(
      CesiumToUnrealTransform * primData.HighPrecisionNodeTransform));

  if (uobject->Mobility == EComponentMobility::Movable) {
    // For movable objects, move the component in the normal way, but don't
    // generate collisions along the way. Teleporting physics is imperfect,
    // but it's the best available option.
    uobject->SetRelativeTransform(
        transform,
        false,
        nullptr,
        ETeleportType::TeleportPhysics);
  } else {
    // Unreal will yell at us for calling SetRelativeTransform on a static
    // object, but we still need to adjust (accurately!) for origin rebasing
    // and georeference changes. It's "ok" to move a static object in this way
    // because, we assume, the globe and globe-oriented lights, etc. are
    // moving too, so in a relative sense the object isn't actually moving.
    // This isn't a perfect assumption, of course.
    uobject->SetRelativeTransform_Direct(transform);
    uobject->UpdateComponentToWorld();
    uobject->MarkRenderTransformDirty();
    pCesiumPrimitive->resetPhysicsTransform();
  }
}
