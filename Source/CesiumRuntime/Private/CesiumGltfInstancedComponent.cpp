// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumGltfInstancedComponent.h"

// Sets default values for this component's properties
UCesiumGltfInstancedComponent::UCesiumGltfInstancedComponent() {
  // Set this component to be initialized when the game starts, and to be ticked
  // every frame.  You can turn these features off to improve performance if you
  // don't need them.
  PrimaryComponentTick.bCanEverTick = false;

  // ...
}

UCesiumGltfInstancedComponent::~UCesiumGltfInstancedComponent() {}

void UCesiumGltfInstancedComponent::UpdateTransformFromCesium(
    const glm::dmat4& CesiumToUnrealTransform) {

  if (this->GetInstanceCount() != InstanceToNodeTransforms.size()) {
    return;
  }

  for (int32 i = 0; i < this->GetInstanceCount(); ++i) {
    glm::dmat4 newInstanceTransform = CesiumToUnrealTransform *
                                      InstanceToNodeTransforms[i] *
                                      this->HighPrecisionNodeTransform;

    this->UpdateInstanceTransform(
        i,
        FTransform(FMatrix(
            FVector(
                newInstanceTransform[0].x,
                newInstanceTransform[0].y,
                newInstanceTransform[0].z),
            FVector(
                newInstanceTransform[1].x,
                newInstanceTransform[1].y,
                newInstanceTransform[1].z),
            FVector(
                newInstanceTransform[2].x,
                newInstanceTransform[2].y,
                newInstanceTransform[2].z),
            FVector(
                newInstanceTransform[3].x,
                newInstanceTransform[3].y,
                newInstanceTransform[3].z))),
        true,
        true,
        true);
  }
}
