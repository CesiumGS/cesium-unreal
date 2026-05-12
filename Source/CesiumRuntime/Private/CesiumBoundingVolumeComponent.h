// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumViewExtension.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "CoreMinimal.h"
#include "PrimitiveSceneProxy.h"
#include "SceneView.h"
#include <Cesium3DTilesSelection/BoundingVolume.h>
#include <Cesium3DTilesSelection/TileOcclusionRendererProxy.h>
#include <glm/mat4x4.hpp>
#include <memory>
#include <optional>
#include "CesiumBoundingVolumeComponent.generated.h"

class ACesiumGeoreference;

UCLASS()
class UCesiumBoundingVolumePoolComponent : public USceneComponent {
  GENERATED_BODY()

public:
  UCesiumBoundingVolumePoolComponent();

  /**
   * Initialize the TileOcclusionRendererProxyPool implementation.
   */
  void initPool(int32 maxPoolSize);

  /**
   * Updates bounding volume transforms from a new double-precision
   * transformation from the Cesium world to the Unreal Engine world.
   *
   * @param CesiumToUnrealTransform The new transformation.
   */
  void UpdateTransformFromCesium(const glm::dmat4& CesiumToUnrealTransform);

  const std::shared_ptr<Cesium3DTilesSelection::TileOcclusionRendererProxyPool>&
  getPool() {
    return this->_pPool;
  }

private:
  glm::dmat4 _cesiumToUnreal;

  // These are really implementations of the functions in
  // TileOcclusionRendererProxyPool, but we can't use multiple inheritance with
  // UObjects. Instead use the CesiumBoundingVolumePool and forward virtual
  // calls to the implementations.

  Cesium3DTilesSelection::TileOcclusionRendererProxy* createProxy();

  void destroyProxy(Cesium3DTilesSelection::TileOcclusionRendererProxy* pProxy);

  class CesiumBoundingVolumePool
      : public Cesium3DTilesSelection::TileOcclusionRendererProxyPool {
  public:
    CesiumBoundingVolumePool(
        UCesiumBoundingVolumePoolComponent* pOutter,
        int32 maxPoolSize);

  protected:
    Cesium3DTilesSelection::TileOcclusionRendererProxy* createProxy() override;

    void destroyProxy(
        Cesium3DTilesSelection::TileOcclusionRendererProxy* pProxy) override;

  private:
    UCesiumBoundingVolumePoolComponent* _pOutter;
  };

  std::shared_ptr<Cesium3DTilesSelection::TileOcclusionRendererProxyPool>
      _pPool;
};

UCLASS()
class UCesiumBoundingVolumeComponent
    : public UPrimitiveComponent,
      public Cesium3DTilesSelection::TileOcclusionRendererProxy {
  GENERATED_BODY()

public:
  // Sets default values for this component's properties
  UCesiumBoundingVolumeComponent(){};
  virtual ~UCesiumBoundingVolumeComponent() = default;

  FPrimitiveSceneProxy* CreateSceneProxy() override;

  /**
   * Update the occlusion state for this bounding volume from the
   * CesiumViewExtension.
   */
  void UpdateOcclusion(const CesiumViewExtension& cesiumViewExtension);

  /**
   * Updates this component's transform from a new double-precision
   * transformation from the Cesium world to the Unreal Engine world, as well as
   * the current tile's transform.
   *
   * @param CesiumToUnrealTransform The new transformation.
   */
  void UpdateTransformFromCesium(const glm::dmat4& CesiumToUnrealTransform);

  virtual FBoxSphereBounds
  CalcBounds(const FTransform& LocalToWorld) const override;

  bool ShouldRecreateProxyOnUpdateTransform() const override { return true; }

  // virtual void BeginDestroy() override;

  Cesium3DTilesSelection::TileOcclusionState
  getOcclusionState() const override {
    return _occlusionState;
  }

protected:
  void reset(const Cesium3DTilesSelection::Tile* pTile) override;

private:
  void _updateTransform();

  Cesium3DTilesSelection::TileOcclusionState _occlusionState =
      Cesium3DTilesSelection::TileOcclusionState::OcclusionUnavailable;

  // Whether this proxy is currently mapped to a tile.
  bool _isMapped = false;

  // The time when this bounding volume was mapped to the tile.
  float _mappedFrameTime = 0.0f;

  Cesium3DTilesSelection::BoundingVolume _tileBounds =
      CesiumGeometry::OrientedBoundingBox(glm::dvec3(0.0), glm::dmat3(1.0));
  glm::dmat4 _tileTransform = glm::dmat4(1.0);
  glm::dmat4 _cesiumToUnreal = glm::dmat4(1.0);
};
