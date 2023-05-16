// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumGltfPointsComponent.h"
#include "CesiumGltfPointsSceneProxy.h"

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
  FPrimitiveSceneProxy* Proxy =
      new FCesiumGltfPointsSceneProxy(this, GetScene()->GetFeatureLevel());
  if (IsValid(this->pTilesetActor)) {
    this->pTilesetActor->_pPointsSceneProxyUpdater->MarkForUpdateNextFrame();
  }
  return Proxy;
}
