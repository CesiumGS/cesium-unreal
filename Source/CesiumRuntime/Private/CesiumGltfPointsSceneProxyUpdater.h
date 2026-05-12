// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "Cesium3DTileset.h"
#include "CesiumGltfPointsComponent.h"
#include "CesiumGltfPointsSceneProxy.h"

/**
 * This is used by Cesium3DTilesets to propagate their settings to any glTF
 * points components it parents.
 */
class FCesiumGltfPointsSceneProxyUpdater {
public:
  /** Updates proxies with new tileset settings. Must be called from a game
   * thread. */
  static void UpdateSettingsInProxies(ACesium3DTileset* Tileset) {
    if (!IsValid(Tileset) || !IsInGameThread()) {
      return;
    }

    TInlineComponentArray<UCesiumGltfPointsComponent*> ComponentArray;
    Tileset->GetComponents<UCesiumGltfPointsComponent>(ComponentArray);

    // Used to pass tileset data updates to render thread
    TArray<FCesiumGltfPointsSceneProxy*> SceneProxies;
    TArray<FCesiumGltfPointsSceneProxyTilesetData> ProxyTilesetData;

    for (UCesiumGltfPointsComponent* PointsComponent : ComponentArray) {
      FCesiumGltfPointsSceneProxy* PointsProxy =
          static_cast<FCesiumGltfPointsSceneProxy*>(
              PointsComponent->SceneProxy);
      if (PointsProxy) {
        SceneProxies.Add(PointsProxy);
      }

      FCesiumGltfPointsSceneProxyTilesetData TilesetData;
      TilesetData.UpdateFromComponent(PointsComponent);
      ProxyTilesetData.Add(TilesetData);
    }

    // Update tileset data
    ENQUEUE_RENDER_COMMAND(TransferCesium3DTilesetSettingsToPointsProxies)
    ([SceneProxies,
      ProxyTilesetData](FRHICommandListImmediate& RHICmdList) mutable {
      // Iterate over proxies and update their data
      for (int32 i = 0; i < SceneProxies.Num(); i++) {
        SceneProxies[i]->UpdateTilesetData(ProxyTilesetData[i]);
      }
    });
  }
};
