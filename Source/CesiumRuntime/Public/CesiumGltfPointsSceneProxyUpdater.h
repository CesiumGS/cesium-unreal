#pragma once

#include "CesiumPointCloudShading.h"
#include "Templates/SharedPointer.h"
#include <glm/vec3.hpp>

class ACesium3DTileset;
class UCesiumGltfPointsComponent;
class FCesiumGltfPointsSceneProxy;

/**
 * Used to pass tile data and Cesium3DTileset settings to a render thread to
 * update the SceneProxy.
 */
struct FCesiumGltfPointsSceneProxyTilesetData {
  FCesiumPointCloudShading PointCloudShading;
  double MaximumScreenSpaceError;
  bool UsesAdditiveRefinement;
  float GeometricError;
  glm::vec3 Dimensions;

  FCesiumGltfPointsSceneProxyTilesetData();

  void UpdateFromComponent(UCesiumGltfPointsComponent* Component);
};

/**
 * This is used by a Cesium3DTileset to propagate its settings to any glTF
 * points components it parents.
 */
class FCesiumGltfPointsSceneProxyUpdater {
public:
  FCesiumGltfPointsSceneProxyUpdater(ACesium3DTileset* Tileset);
  ~FCesiumGltfPointsSceneProxyUpdater();

  /** Updates proxies with new tileset settings. Must be called from a game
   * thread. */
  void UpdateSettingsInProxies();

  /** Updates only if marked as needing an update. */
  void UpdateSettingsInProxiesIfNeeded();

  /** Marks this as needing an update next frame. This is called from
   * CreateSceneProxy because the proxy won't be assigned to the primitive
   * component until after that function. */
  void MarkForUpdateNextFrame();

private:
  ACesium3DTileset* Owner;
  bool bNeedsUpdate;

  void TransferSettingsToProxies();
};
