// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumGltfLinesComponent.h"
#include "CesiumGltfLinesSceneProxy.h"
#include "SceneInterface.h"

// Sets default values for this component's properties
UCesiumGltfLinesComponent::UCesiumGltfLinesComponent() {}

UCesiumGltfLinesComponent::~UCesiumGltfLinesComponent() {}

FPrimitiveSceneProxy* UCesiumGltfLinesComponent::CreateSceneProxy() {
  if (!IsValid(this)) {
    return nullptr;
  }

  return new FCesiumGltfLinesSceneProxy(this, GetScene()->GetFeatureLevel());
}
