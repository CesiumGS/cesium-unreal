// Copyright 2020-2026 CesiumGS, Inc. and Contributors

#include "CesiumGltfInstancedComponent.h"
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

UCesiumGltfInstancedComponent::UCesiumGltfInstancedComponent() {
  PrimaryComponentTick.bCanEverTick = false;
}

PRAGMA_ENABLE_DEPRECATION_WARNINGS

UCesiumGltfInstancedComponent::~UCesiumGltfInstancedComponent() {}

void UCesiumGltfInstancedComponent::BeginDestroy() {
  // Clear everything we can in order to reduce memory usage, because this
  // UObject might not actually get deleted by the garbage collector until
  // much later.
  ICesiumPrimitive* pCesiumPrimitive = Cast<ICesiumPrimitive>(this);
  pCesiumPrimitive->getPrimitiveData().destroy();

  if (UMaterialInstanceDynamic* pMaterial =
          Cast<UMaterialInstanceDynamic>(this->GetMaterial(0))) {
    CesiumLifetime::destroy(pMaterial);
  }

  if (UStaticMesh* pMesh = this->GetStaticMesh()) {
    if (UBodySetup* pBodySetup = pMesh->GetBodySetup()) {
      CesiumLifetime::destroy(pBodySetup);
    }
    CesiumLifetime::destroy(pMesh);
  }

  Super::BeginDestroy();
}

FBoxSphereBounds UCesiumGltfInstancedComponent::CalcBounds(
    const FTransform& LocalToWorld) const {
  const CesiumPrimitiveData& primitiveData = this->getPrimitiveData();
  if (!primitiveData.boundingVolume) {
    return Super::CalcBounds(LocalToWorld);
  }

  std::optional<FBoxSphereBounds> maybeBounds = std::visit(
      CalcBoundsOperation{
          LocalToWorld,
          primitiveData.highPrecisionNodeTransform},
      *primitiveData.boundingVolume);
  return maybeBounds.value_or(Super::CalcBounds(LocalToWorld));
}

void UCesiumGltfInstancedComponent::UpdateTransformFromCesium(
    const glm::dmat4& CesiumToUnrealTransform) {
  const CesiumPrimitiveData& primitiveData = this->getPrimitiveData();
  const FTransform transform = VecMath::createTransform(
      CesiumToUnrealTransform * primitiveData.highPrecisionNodeTransform);

  if (this->Mobility == EComponentMobility::Movable) {
    // For movable objects, move the component in the normal way, but don't
    // generate collisions along the way. Teleporting physics is imperfect,
    // but it's the best available option.
    this->SetRelativeTransform(
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
    this->SetRelativeTransform_Direct(transform);
    this->UpdateComponentToWorld();
    this->MarkRenderTransformDirty();
    SendPhysicsTransform(ETeleportType::ResetPhysics);
  }
}

CesiumPrimitiveData& UCesiumGltfInstancedComponent::getPrimitiveData() {
  return this->_cesiumData;
}

const CesiumPrimitiveData&
UCesiumGltfInstancedComponent::getPrimitiveData() const {
  return this->_cesiumData;
}

UStaticMeshComponent& UCesiumGltfInstancedComponent::GetMeshComponent() {
  return *this;
}

ICesiumLoadedTile& UCesiumGltfInstancedComponent::GetLoadedTile() {
  // Not GetAttachParent(): not yet attached (eg. when calling
  // ICesium3DTilesetLifecycleEventReceiver::CreateMaterial)
  return *Cast<ICesiumLoadedTile>(GetOuter());
}

std::optional<uint32_t>
UCesiumGltfInstancedComponent::FindTextureCoordinateIndexForGltfAccessor(
    int32_t accessorIndex) const {
  std::unordered_map<int32_t, uint32_t> texCoordMap =
      this->getPrimitiveData().gltfToUnrealTexCoordMap;
  auto uvIndexIt = texCoordMap.find(accessorIndex);
  return uvIndexIt != texCoordMap.end() ? std::make_optional(uvIndexIt->second)
                                        : std::nullopt;
}
