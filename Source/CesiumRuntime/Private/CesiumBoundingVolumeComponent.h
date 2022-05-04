// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Components/PrimitiveComponent.h"	
#include "Components/SceneComponent.h"
#include "PrimitiveSceneProxy.h"
#include "CoreMinimal.h"
#include <glm/mat4x4.hpp>
#include <Cesium3DTilesSelection/BoundingVolume.h>
#include <Cesium3DTilesSelection/TileOcclusionRendererProxy.h>
#include <optional>
#include <memory>
#include "CesiumBoundingVolumeComponent.generated.h"

class ACesiumGeoreference;

UCLASS()
class UCesiumBoundingVolumePoolComponent : 
public USceneComponent {
  GENERATED_BODY()

public:
  UCesiumBoundingVolumePoolComponent();

  /**
   * Updates bounding volume transforms from a new double-precision
   * transformation from the Cesium world to the Unreal Engine world.
   *
   * @param CesiumToUnrealTransform The new transformation.
   */
  void UpdateTransformFromCesium(const glm::dmat4& CesiumToUnrealTransform);

  const std::shared_ptr<Cesium3DTilesSelection::TileOcclusionRendererProxyPool>& getPool() {
    return this->_pPool;
  }

private:
  // These are really implementations of the functions in 
  // TileOcclusionRendererProxyPool, but we can't use multiple inheritance with
  // UObjects. Instead use the CesiumBoundingVolumePool and forward virtual 
  // calls to the implementations.
  friend class UCesiumBoundingVolumePool;

  Cesium3DTilesSelection::TileOcclusionRendererProxy* createProxy();

  void destroyProxy(
      Cesium3DTilesSelection::TileOcclusionRendererProxy* pProxy);

  std::shared_ptr<Cesium3DTilesSelection::TileOcclusionRendererProxyPool> _pPool;
};

class UCesiumBoundingVolumePool : public Cesium3DTilesSelection::TileOcclusionRendererProxyPool {
public:
  UCesiumBoundingVolumePool(UCesiumBoundingVolumePoolComponent* pOutter);

protected:
  Cesium3DTilesSelection::TileOcclusionRendererProxy* createProxy() override;

  void destroyProxy(
      Cesium3DTilesSelection::TileOcclusionRendererProxy* pProxy) override;

private:
  UCesiumBoundingVolumePoolComponent* _pOutter;
};

UCLASS()
class UCesiumBoundingVolumeComponent : 
public UPrimitiveComponent, public Cesium3DTilesSelection::TileOcclusionRendererProxy {
  GENERATED_BODY()

public:
  // Sets default values for this component's properties
  UCesiumBoundingVolumeComponent();
  virtual ~UCesiumBoundingVolumeComponent() = default;

  FPrimitiveSceneProxy* CreateSceneProxy() override;

  /**
   * Set the occlusion result for this bounding volume, or nullopt if there 
   * was not a definitive occlusion result this frame.
   * 
   * @param isOccluded The occlusion result.
   */
  void SetOcclusionResult_RenderThread(const std::optional<bool>& isOccluded);
  
  /**
   * Notifies that the render thread occlusion retrieval task is complete and 
   * has not been re-queued up yet. Upon receiving this notification, the 
   * component will save the render thread result onto a game thread 
   * reflection. Called on the game thread. 
   */
  void SyncOcclusionResult();

  /**
   * Updates this component's transform from a new double-precision
   * transformation from the Cesium world to the Unreal Engine world, as well as
   * the current tile's transform.
   *
   * @param CesiumToUnrealTransform The new transformation.
   */
  void UpdateTransformFromCesium(const glm::dmat4& CesiumToUnrealTransform);

  //virtual void BeginDestroy() override;

// TileOcclusionRendererProxy implementation
  bool isOccluded() const override;

  int32_t getLastUpdatedFrame() const override;

protected:
  void reset(const Cesium3DTilesSelection::Tile* pTile) override;

  void update(int32_t currentFrame) override;

private: 
  void _updateTransform();
  
  // Updated in render thread 
  std::optional<bool> _isOccluded_RenderThread;

  // Game thread safe information. Synced from the render thread result
  bool _isOccluded = false;
  bool _occlusionUpdatedThisFrame = false;
  int32_t _lastUpdatedFrame = -1000;
  
  // TODO:
  // Do these need to be accessed on the game thread, render thread?
  // Do they get reloaded on render state reload? 
  Cesium3DTilesSelection::BoundingVolume _localTileBounds = 
      CesiumGeometry::OrientedBoundingBox(
        glm::dvec3(0.0), glm::dmat3(1.0));
  glm::dmat4 _tileTransform;
  glm::dmat4 _cesiumToUnreal;
};
