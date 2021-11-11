// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumLifetime.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"

// Sets default values for this component's properties
UCesiumGltfPrimitiveComponent::UCesiumGltfPrimitiveComponent() {
  // Set this component to be initialized when the game starts, and to be ticked
  // every frame.  You can turn these features off to improve performance if you
  // don't need them.
  PrimaryComponentTick.bCanEverTick = false;
  pModel = nullptr;
  pMeshPrimitive = nullptr;
}

UCesiumGltfPrimitiveComponent::~UCesiumGltfPrimitiveComponent() {}

void UCesiumGltfPrimitiveComponent::UpdateTransformFromCesium(
    const glm::dmat4& CesiumToUnrealTransform) {
  this->SetUsingAbsoluteLocation(true);
  this->SetUsingAbsoluteRotation(true);
  this->SetUsingAbsoluteScale(true);

  const glm::dmat4x4& transform =
      CesiumToUnrealTransform * this->HighPrecisionNodeTransform;

  this->SetRelativeTransform(FTransform(FMatrix(
      FVector(transform[0].x, transform[0].y, transform[0].z),
      FVector(transform[1].x, transform[1].y, transform[1].z),
      FVector(transform[2].x, transform[2].y, transform[2].z),
      FVector(transform[3].x, transform[3].y, transform[3].z))));
}

void UCesiumGltfPrimitiveComponent::BeginDestroy() {
  UStaticMesh* pMesh = this->GetStaticMesh();
  if (pMesh) {
    UMaterialInterface* pMaterial = pMesh->GetMaterial(0);
    if (pMaterial) {
      UTexture* pBaseColorTexture = nullptr;
      if (pMaterial->GetTextureParameterValue(
              FMaterialParameterInfo(
                  "baseColorTexture",
                  EMaterialParameterAssociation::LayerParameter,
                  0),
              pBaseColorTexture,
              true)) {
        CesiumLifetime::destroy(pBaseColorTexture);
      }

      // TODO: destroy other textures

      CesiumLifetime::destroy(pMaterial);
    }

    if (pMesh->BodySetup) {
      CesiumLifetime::destroy(pMesh->BodySetup);
    }

    CesiumLifetime::destroy(pMesh);
  }

  Super::BeginDestroy();
}
