// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumGltfPrimitiveComponent.h"
#include "CalcBounds.h"
#include "CesiumLifetime.h"
#include "CesiumMaterialUserData.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PhysicsEngine/BodySetup.h"
#include "VecMath.h"
#include "Engine/Texture.h"
#include <variant>

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

  const FTransform transform = FTransform(VecMath::createMatrix(
      CesiumToUnrealTransform * this->HighPrecisionNodeTransform));

  this->SetRelativeTransform_Direct(transform);
  this->SetComponentToWorld(transform);
  this->MarkRenderTransformDirty();
}

namespace {

void destroyMaterialTexture(
    UMaterialInstanceDynamic* pMaterial,
    const char* name,
    EMaterialParameterAssociation assocation,
    int32 index) {
  UTexture* pTexture = nullptr;
  if (pMaterial->GetTextureParameterValue(
          FMaterialParameterInfo(name, assocation, index),
          pTexture,
          true)) {
    FTexturePlatformData** RunningPlatformDataPtr = pTexture->GetRunningPlatformData();

    if (RunningPlatformDataPtr) {
      FTexturePlatformData*& RunningPlatformData = *RunningPlatformDataPtr;

      if (RunningPlatformData != NULL) {
        delete RunningPlatformData;
        RunningPlatformData = NULL;
      }
    }
    CesiumLifetime::destroy(pTexture);
  }
}

void destroyGltfParameterValues(
    UMaterialInstanceDynamic* pMaterial,
    EMaterialParameterAssociation assocation,
    int32 index) {
  destroyMaterialTexture(pMaterial, "baseColorTexture", assocation, index);
  destroyMaterialTexture(
      pMaterial,
      "metallicRoughnessTexture",
      assocation,
      index);
  destroyMaterialTexture(pMaterial, "normalTexture", assocation, index);
  destroyMaterialTexture(pMaterial, "emissiveTexture", assocation, index);
  destroyMaterialTexture(pMaterial, "occlusionTexture", assocation, index);
}

void destroyWaterParameterValues(
    UMaterialInstanceDynamic* pMaterial,
    EMaterialParameterAssociation assocation,
    int32 index) {
  destroyMaterialTexture(pMaterial, "WaterMask", assocation, index);
}
} // namespace

void UCesiumGltfPrimitiveComponent::BeginDestroy() {
  // This should mirror the logic in loadPrimitiveGameThreadPart in
  // CesiumGltfComponent.cpp
  UMaterialInstanceDynamic* pMaterial =
      Cast<UMaterialInstanceDynamic>(this->GetMaterial(0));
  if (pMaterial) {

    destroyGltfParameterValues(
        pMaterial,
        EMaterialParameterAssociation::GlobalParameter,
        INDEX_NONE);
    destroyWaterParameterValues(
        pMaterial,
        EMaterialParameterAssociation::GlobalParameter,
        INDEX_NONE);

    UMaterialInterface* pBaseMaterial = pMaterial->Parent;
    UMaterialInstance* pBaseAsMaterialInstance =
        Cast<UMaterialInstance>(pBaseMaterial);
    UCesiumMaterialUserData* pCesiumData =
        pBaseAsMaterialInstance
            ? pBaseAsMaterialInstance
                  ->GetAssetUserData<UCesiumMaterialUserData>()
            : nullptr;
    if (pCesiumData) {
      destroyGltfParameterValues(
          pMaterial,
          EMaterialParameterAssociation::LayerParameter,
          0);

      int32 waterIndex = pCesiumData->LayerNames.Find("Water");
      if (waterIndex >= 0) {
        destroyWaterParameterValues(
            pMaterial,
            EMaterialParameterAssociation::LayerParameter,
            waterIndex);
      }
    }

    CesiumEncodedMetadataUtility::destroyEncodedMetadataPrimitive(
        this->EncodedMetadata);

    CesiumLifetime::destroy(pMaterial);
  }

  UStaticMesh* pMesh = this->GetStaticMesh();
  if (pMesh) {
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 27
    UBodySetup* pBodySetup = pMesh->BodySetup;
#else
    UBodySetup* pBodySetup = pMesh->GetBodySetup();
#endif
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
