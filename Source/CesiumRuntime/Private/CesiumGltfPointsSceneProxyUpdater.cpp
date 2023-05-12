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

FCesiumGltfPointsSceneProxyUpdater::~FCesiumGltfPointsSceneProxyUpdater() {
  this->RegisteredProxies.Empty();
}

FCesiumGltfPointsSceneProxyUpdater::FCesiumRegisteredProxy::
    FCesiumRegisteredProxy(
        TWeakObjectPtr<UCesiumGltfPointsComponent> Component,
        TWeakPtr<FCesiumGltfPointsSceneProxyWrapper, ESPMode::ThreadSafe>
            SceneProxyWrapper)
    : Component(Component),
      Tileset(Component->pTilesetActor),
      SceneProxyWrapper(SceneProxyWrapper) {}

void FCesiumGltfPointsSceneProxyUpdater::RegisterProxy(
    UCesiumGltfPointsComponent* Component,
    TWeakPtr<FCesiumGltfPointsSceneProxyWrapper, ESPMode::ThreadSafe>
        SceneProxyWrapper) {
  if (IsValid(Component)) {
    FCriticalSection ProxyLock;

    if (TSharedPtr<FCesiumGltfPointsSceneProxyWrapper, ESPMode::ThreadSafe>
            SceneProxyWrapperShared = SceneProxyWrapper.Pin()) {
      FScopeLock Lock(&ProxyLock);
      this->RegisteredProxies.Emplace(Component, SceneProxyWrapper);
      this->UpdateSettingsInProxies();
    }
  }
}

void FCesiumGltfPointsSceneProxyUpdater::PrepareProxies() {
  for (int32 i = 0; i < this->RegisteredProxies.Num(); ++i) {
    FCesiumRegisteredProxy& RegisteredProxy = this->RegisteredProxies[i];
    bool bValidProxy = false;

    // Acquire a Shared Pointer from the Weak Pointer and check that it
    // references a valid object
    if (TSharedPtr<FCesiumGltfPointsSceneProxyWrapper, ESPMode::ThreadSafe>
            SceneProxyWrapper = RegisteredProxy.SceneProxyWrapper.Pin()) {
      if (UCesiumGltfPointsComponent* Component =
              RegisteredProxy.Component.Get()) {
        if (ACesium3DTileset* Tileset = RegisteredProxy.Tileset.Get()) {
          if (Tileset == this->Owner) {
            RegisteredProxy.TilesetData.UpdateFromComponent(Component);
            bValidProxy = true;
          }
        }
      }
    }

    // If the SceneProxy has been destroyed, remove it from the list and
    // reiterate
    if (!bValidProxy) {
      RegisteredProxies.RemoveAtSwap(i--, 1, false);
    }
  }
}

void FCesiumGltfPointsSceneProxyUpdater::TransferSettingsToProxies() {
  if (RegisteredProxies.Num() == 0) {
    return;
  }

  // Used to pass render data updates to render thread
  TArray<FCesiumGltfPointsSceneProxyTilesetData> ProxyTilesetData;

  // Process Nodes
  {
    for (int32 i = 0; i < RegisteredProxies.Num(); ++i) {
      const FCesiumRegisteredProxy& RegisteredProxy = RegisteredProxies[i];

      // Attempt to get a data lock
      FScopeTryLock TryDataLock(&this->DataLock);
      if (TryDataLock.IsLocked()) {
        FCesiumGltfPointsSceneProxyTilesetData TilesetData =
            RegisteredProxy.TilesetData;
        TilesetData.SceneProxyWrapper = RegisteredProxy.SceneProxyWrapper;
        ProxyTilesetData.Add(TilesetData);
      }
    }
  }

  // Update Render Data
  ENQUEUE_RENDER_COMMAND(TransferCesium3DTilesetSettingsToPointsProxies)
  ([ProxyTilesetData](FRHICommandListImmediate& RHICmdList) mutable {
    // Iterate over proxies and, if valid, update their data
    for (FCesiumGltfPointsSceneProxyTilesetData& TilesetData :
         ProxyTilesetData) {
      // Check for proxy's validity, in case it has been destroyed since the
      // update was issued
      if (TSharedPtr<FCesiumGltfPointsSceneProxyWrapper, ESPMode::ThreadSafe>
              SceneProxyWrapper = TilesetData.SceneProxyWrapper.Pin()) {
        if (SceneProxyWrapper->Proxy) {
          SceneProxyWrapper->Proxy->UpdateTilesetData(TilesetData);
        }
      }
    }
  });
}

void FCesiumGltfPointsSceneProxyUpdater::UpdateSettingsInProxies() {
  if (IsInGameThread()) {
    PrepareProxies();
    TransferSettingsToProxies();
  }
}
