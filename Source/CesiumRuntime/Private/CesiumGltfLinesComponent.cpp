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

  return new FCesiumGltfLinesSceneProxy(
      this,
      FSceneInterfaceWrapper(GetScene()));
}

void UCesiumGltfLinesComponent::OnCreatePhysicsState() {
  // The workaround for small-scale geometry in UCesiumGltfPrimitiveComponent
  // does not seem to work for non-triangle geometry, which can result in
  // warnings spammed to the console. Therefore, we skip the
  // Super::OnCreatePhysicsState chain entirely.
  // We can get away with this by setting this variable that would have
  // otherwise been set by UActorComponent::OnCreatePhysicsState().
  this->bPhysicsStateCreated = true;
}
