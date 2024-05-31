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
void destroyCesiumPrimitive(UStaticMeshComponent* pComponent) {
  // Clear everything we can in order to reduce memory usage, because this
  // UObject might not actually get deleted by the garbage collector until
  // much later.
  auto* cesiumPrimitive = Cast<ICesiumPrimitive>(pComponent);
  cesiumPrimitive->getPrimitiveData().destroy();
  UMaterialInstanceDynamic* pMaterial =
      Cast<UMaterialInstanceDynamic>(pComponent->GetMaterial(0));
  if (pMaterial) {
    CesiumLifetime::destroy(pMaterial);
  }

  UStaticMesh* pMesh = pComponent->GetStaticMesh();
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

namespace {

std::optional<FBoxSphereBounds> calcBounds(
    const ICesiumPrimitive& primitive, const FTransform& LocalToWorld) {
  const CesiumPrimitiveData& primData = primitive.getPrimitiveData();
  if (!primData.boundingVolume) {
    return std::nullopt;
  }

  return std::visit(
      CalcBoundsOperation{
          LocalToWorld,
          primData.HighPrecisionNodeTransform},
      *primData.boundingVolume);
}

} // namespace

FBoxSphereBounds UCesiumGltfPrimitiveComponent::CalcBounds(
    const FTransform& LocalToWorld) const {
  if (auto bounds = calcBounds(*this, LocalToWorld)) {
    return *bounds;
  }
  return Super::CalcBounds(LocalToWorld);
}

FBoxSphereBounds UCesiumGltfInstancedComponent::CalcBounds(
    const FTransform& LocalToWorld) const {
  if (auto bounds = calcBounds(*this, LocalToWorld)) {
    return *bounds;
  }
  return Super::CalcBounds(LocalToWorld);
}

namespace {
// Returns true if the component is moveable, so caller can "Do The Right
// Thing." This avoids making the protected member function SendPhysicsTransform
// public.
template <typename CesiumComponent>
bool UpdateTransformFromCesiumAux(
    const glm::dmat4& CesiumToUnrealTransform,
    CesiumComponent* cesiumComponent) {
  const CesiumPrimitiveData& primData = cesiumComponent->getPrimitiveData();
  const FTransform transform = FTransform(VecMath::createMatrix(
      CesiumToUnrealTransform * primData.HighPrecisionNodeTransform));

  if (cesiumComponent->Mobility == EComponentMobility::Movable) {
    // For movable objects, move the component in the normal way, but don't
    // generate collisions along the way. Teleporting physics is imperfect,
    // but it's the best available option.
    cesiumComponent->SetRelativeTransform(
        transform,
        false,
        nullptr,
        ETeleportType::TeleportPhysics);
    return true;
  }
  // Unreal will yell at us for calling SetRelativeTransform on a static
  // object, but we still need to adjust (accurately!) for origin rebasing
  // and georeference changes. It's "ok" to move a static object in this way
  // because, we assume, the globe and globe-oriented lights, etc. are
  // moving too, so in a relative sense the object isn't actually moving.
  // This isn't a perfect assumption, of course.
  cesiumComponent->SetRelativeTransform_Direct(transform);
  cesiumComponent->UpdateComponentToWorld();
  cesiumComponent->MarkRenderTransformDirty();
  return false;
  }
} // namespace

void UCesiumGltfPrimitiveComponent::UpdateTransformFromCesium(
    const glm::dmat4& CesiumToUnrealTransform)
{
  bool moveable = UpdateTransformFromCesiumAux(CesiumToUnrealTransform, this);
  if (!moveable) {
    SendPhysicsTransform(ETeleportType::ResetPhysics);
  }
}

void UCesiumGltfInstancedComponent::UpdateTransformFromCesium(
    const glm::dmat4& CesiumToUnrealTransform)
{
  bool moveable = UpdateTransformFromCesiumAux(CesiumToUnrealTransform, this);
  if (!moveable) {
    SendPhysicsTransform(ETeleportType::ResetPhysics);
  }
}

CesiumPrimitiveData& UCesiumGltfPrimitiveComponent::getPrimitiveData() {
  return _cesiumData;
}

const CesiumPrimitiveData& UCesiumGltfPrimitiveComponent::getPrimitiveData() const {
  return _cesiumData;
}

CesiumPrimitiveData& UCesiumGltfInstancedComponent::getPrimitiveData() {
  return _cesiumData;
}

const CesiumPrimitiveData& UCesiumGltfInstancedComponent::getPrimitiveData() const {
  return _cesiumData;
}
