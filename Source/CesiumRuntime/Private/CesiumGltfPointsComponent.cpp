// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumGltfPointsComponent.h"
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
      new FCesiumGltfPointsSceneProxy(this, GetScene()->GetFeatureLevel());

  FCesiumGltfPointsSceneProxyTilesetData TilesetData;
  TilesetData.UpdateFromComponent(this);
  Proxy->UpdateTilesetData(TilesetData);

  return Proxy;
}
