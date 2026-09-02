// Copyright 2020-2026 CesiumGS, Inc. and Contributors

#include "CesiumGltfVoxelComponent.h"

// Sets default values for this component's properties
UCesiumGltfVoxelComponent::UCesiumGltfVoxelComponent() {
  PrimaryComponentTick.bCanEverTick = false;
}

UCesiumGltfVoxelComponent::~UCesiumGltfVoxelComponent() {}

void UCesiumGltfVoxelComponent::BeginDestroy() { Super::BeginDestroy(); }
