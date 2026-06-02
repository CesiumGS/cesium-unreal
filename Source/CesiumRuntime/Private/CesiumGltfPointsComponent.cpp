// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumGltfPointsComponent.h"
#include "CesiumCompat.h"
#include "CesiumGltfPointsSceneProxy.h"
#include "SceneInterface.h"

// Sets default values for this component's properties
UCesiumGltfPointsComponent::UCesiumGltfPointsComponent()
    : UsesAdditiveRefinement(false),
      GeometricError(0),
      Dimensions(glm::vec3(0)) {}

UCesiumGltfPointsComponent::~UCesiumGltfPointsComponent() {}

FPrimitiveSceneProxy* UCesiumGltfPointsComponent::CreateSceneProxy() {
  if (!IsValid(this)) {
    return nullptr;
  }

  FCesiumGltfPointsSceneProxy* Proxy =
      new FCesiumGltfPointsSceneProxy(this, FSceneInterfaceWrapper(GetScene()));

  FCesiumGltfPointsSceneProxyTilesetData TilesetData;
  TilesetData.UpdateFromComponent(this);
  Proxy->UpdateTilesetData(TilesetData);

  return Proxy;
}

void UCesiumGltfPointsComponent::OnCreatePhysicsState() {
  // The workaround for small-scale geometry in UCesiumGltfPrimitiveComponent
  // does not seem to work for non-triangle geometry, which can result in
  // warnings spammed to the console. Therefore, we skip the
  // Super::OnCreatePhysicsState chain entirely.
  // We can get away with this by setting this variable that would have
  // otherwise been set by UActorComponent::OnCreatePhysicsState().
  this->bPhysicsStateCreated = true;
}
