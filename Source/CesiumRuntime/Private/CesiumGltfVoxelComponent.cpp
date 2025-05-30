// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumGltfVoxelComponent.h"

// Sets default values for this component's properties
UCesiumGltfVoxelComponent::UCesiumGltfVoxelComponent() {
  PrimaryComponentTick.bCanEverTick = false;
}

UCesiumGltfVoxelComponent::~UCesiumGltfVoxelComponent() {}

void UCesiumGltfVoxelComponent::BeginDestroy() {
  this->attributeBuffers.Empty();

  Super::BeginDestroy();
}
