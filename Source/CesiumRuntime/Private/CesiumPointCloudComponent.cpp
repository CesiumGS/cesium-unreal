// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumPointCloudComponent.h"

// Sets default values for this component's properties
UCesiumPointCloudComponent::UCesiumPointCloudComponent() {
  PrimaryComponentTick.bCanEverTick = false;
  pModel = nullptr;
  pMeshPrimitive = nullptr;
}

UCesiumPointCloudComponent::~UCesiumPointCloudComponent() {}

void UCesiumPointCloudComponent::UpdateTransformFromCesium(
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
