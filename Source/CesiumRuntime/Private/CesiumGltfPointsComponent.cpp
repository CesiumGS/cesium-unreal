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
  FCesiumGltfPointsSceneProxy* Proxy = new FCesiumGltfPointsSceneProxy(this, GetScene()->GetFeatureLevel());
  this->pTilesetActor->_pPointsSceneProxyUpdater->RegisterProxy(
      this,
      Proxy->ProxyWrapper);
  return Proxy;
}
