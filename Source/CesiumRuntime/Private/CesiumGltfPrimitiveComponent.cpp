// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumGltfPrimitiveComponent.h"
#include "Cesium3DTiles/Tile.h"

// Sets default values for this component's properties
UCesiumGltfPrimitiveComponent::UCesiumGltfPrimitiveComponent() {
  // Set this component to be initialized when the game starts, and to be ticked
  // every frame.  You can turn these features off to improve performance if you
  // don't need them.
  PrimaryComponentTick.bCanEverTick = false;

  // ...
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

FPrimitiveSceneProxy* UCesiumGltfPrimitiveComponent::CreateSceneProxy() {
  UE_LOG(
      LogCesium,
      Display,
      TEXT("CreateSceneProxy %s"),
      UTF8_TO_TCHAR(Cesium3DTiles::TileIdUtilities::createTileIdString(
                        this->pTile->getTileID())
                        .c_str()));
  FPrimitiveSceneProxy* pResult = Super::CreateSceneProxy();
  if (pResult == nullptr) {
    UE_LOG(LogCesium, Display, TEXT("Proxy is nullptr!"));
  }
  return pResult;
}
