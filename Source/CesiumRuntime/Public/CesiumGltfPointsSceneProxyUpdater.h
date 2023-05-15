#pragma once

#include "CesiumPointCloudShading.h"
#include "Templates/SharedPointer.h"
#include <glm/vec3.hpp>

class ACesium3DTileset;
class UCesiumGltfPointsComponent;
class FCesiumGltfPointsSceneProxy;
struct FCesiumGltfPointsSceneProxyWrapper;

/** Used to pass tile data and Cesium3DTileset settings to a render thread to
 * update the SceneProxy.*/
struct FCesiumGltfPointsSceneProxyTilesetData {
  TWeakPtr<FCesiumGltfPointsSceneProxyWrapper, ESPMode::ThreadSafe>
      SceneProxyWrapper;

  FCesiumPointCloudShading PointCloudShading;
  double MaximumScreenSpaceError;
  bool UsesAdditiveRefinement;
  float GeometricError;
  glm::vec3 Dimensions;

  FCesiumGltfPointsSceneProxyTilesetData();

  void UpdateFromComponent(UCesiumGltfPointsComponent* Component);
};

/** This allows access to the SceneProxy via weak pointer */
struct FCesiumGltfPointsSceneProxyWrapper {
  FCesiumGltfPointsSceneProxy* Proxy;
  FCesiumGltfPointsSceneProxyWrapper(FCesiumGltfPointsSceneProxy* Proxy)
      : Proxy(Proxy) {}
};

/** This is used by a Cesium3DTileset to propagate its settings to any glTF
 * points components it parents. */
class FCesiumGltfPointsSceneProxyUpdater {
public:
  FCesiumGltfPointsSceneProxyUpdater(ACesium3DTileset* Tileset);
  ~FCesiumGltfPointsSceneProxyUpdater();

  /** Used for thread safety between rendering and asset operations. */
  mutable FCriticalSection DataLock;

  /** Acts as a bridge between the game thread and render thread. Allows
   * component / tileset data to be copied to the render thread. */
  struct FCesiumRegisteredProxy {
    TWeakObjectPtr<UCesiumGltfPointsComponent> Component;
    TWeakObjectPtr<ACesium3DTileset> Tileset;
    TWeakPtr<FCesiumGltfPointsSceneProxyWrapper, ESPMode::ThreadSafe>
        SceneProxyWrapper;
    FCesiumGltfPointsSceneProxyTilesetData TilesetData;

    FCesiumRegisteredProxy(
        TWeakObjectPtr<UCesiumGltfPointsComponent> Component,
        TWeakPtr<FCesiumGltfPointsSceneProxyWrapper, ESPMode::ThreadSafe>
            SceneProxyWrapper);
  };

  void RegisterProxy(
      UCesiumGltfPointsComponent* Component,
      TWeakPtr<FCesiumGltfPointsSceneProxyWrapper, ESPMode::ThreadSafe>
          SceneProxyWrapper);

  /** List of currently registered glTF point proxies. Used for propagating
   * Cesium3DTileset settings to the scene proxies in a render thread. */
  TArray<FCesiumRegisteredProxy> RegisteredProxies;

  /** Updates the tileset settings. Must be called from a game thread. */
  void UpdateSettingsInProxies();

private:
  ACesium3DTileset* Owner;
  /** Called to prepare the glTF point proxies for processing */
  void PrepareProxies();
  void TransferSettingsToProxies();
};
