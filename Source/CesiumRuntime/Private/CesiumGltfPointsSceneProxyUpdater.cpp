#include "CesiumGltfPointsSceneProxyUpdater.h"

#include "Cesium3DTileset.h"
#include "CesiumGltfPointsComponent.h"
#include "CesiumGltfPointsSceneProxy.h"
#include "Misc/ScopeLock.h"
#include "Misc/ScopeTryLock.h"

FCesiumGltfPointsSceneProxyTilesetData::FCesiumGltfPointsSceneProxyTilesetData()
    : PointCloudShading(),
      MaximumScreenSpaceError(0.0),
      UsesAdditiveRefinement(false),
      GeometricError(0.0f),
      Dimensions() {}

void FCesiumGltfPointsSceneProxyTilesetData::UpdateFromComponent(
    UCesiumGltfPointsComponent* Component) {
  ACesium3DTileset* Tileset = Component->pTilesetActor;
  PointCloudShading = Tileset->GetPointCloudShading();
  MaximumScreenSpaceError = Tileset->MaximumScreenSpaceError;
  UsesAdditiveRefinement = Component->UsesAdditiveRefinement;
  GeometricError = Component->GeometricError;
  Dimensions = Component->Dimensions;
}

FCesiumGltfPointsSceneProxyUpdater::FCesiumGltfPointsSceneProxyUpdater(
    ACesium3DTileset* Tileset)
    : Owner(Tileset) {}

FCesiumGltfPointsSceneProxyUpdater::~FCesiumGltfPointsSceneProxyUpdater() {}

void FCesiumGltfPointsSceneProxyUpdater::MarkForUpdateNextFrame() {
  bNeedsUpdate = true;
}

void FCesiumGltfPointsSceneProxyUpdater::UpdateSettingsInProxies() {
  if (IsValid(this->Owner) && IsInGameThread()) {
    TransferSettingsToProxies();
    bNeedsUpdate = false;
  }
}

void FCesiumGltfPointsSceneProxyUpdater::UpdateSettingsInProxiesIfNeeded() {
  if (bNeedsUpdate) {
    UpdateSettingsInProxies();
  }
}

void FCesiumGltfPointsSceneProxyUpdater::TransferSettingsToProxies() {
  TInlineComponentArray<UActorComponent*> ComponentArray(Owner, true);
  Owner->GetComponents(
      UCesiumGltfPointsComponent::StaticClass(),
      ComponentArray,
      true);

  // Used to pass tileset data updates to render thread
  TArray<FCesiumGltfPointsSceneProxy*> SceneProxies;
  TArray<FCesiumGltfPointsSceneProxyTilesetData> ProxyTilesetData;

  for (UActorComponent* ActorComponent : ComponentArray) {
    UCesiumGltfPointsComponent* PointsComponent =
        static_cast<UCesiumGltfPointsComponent*>(ActorComponent);
    FCesiumGltfPointsSceneProxy* PointsProxy =
        static_cast<FCesiumGltfPointsSceneProxy*>(PointsComponent->SceneProxy);
    if (PointsProxy) {
      SceneProxies.Add(PointsProxy);
    }

    FCesiumGltfPointsSceneProxyTilesetData TilesetData;
    TilesetData.UpdateFromComponent(PointsComponent);
    ProxyTilesetData.Add(TilesetData);
  }

  // Update render Data
  ENQUEUE_RENDER_COMMAND(TransferCesium3DTilesetSettingsToPointsProxies)
  ([SceneProxies,
    ProxyTilesetData](FRHICommandListImmediate& RHICmdList) mutable {
    // Iterate over proxies and, if valid, update their data
    for (int32 i = 0; i < SceneProxies.Num(); i++) {
      FCesiumGltfPointsSceneProxy* SceneProxy = SceneProxies[i];
      if (SceneProxy) {
        SceneProxy->UpdateTilesetData(ProxyTilesetData[i]);
      }
    }
  });
}
