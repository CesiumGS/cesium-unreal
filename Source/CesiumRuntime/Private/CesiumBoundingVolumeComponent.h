// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

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
    CesiumBoundingVolumePool(UCesiumBoundingVolumePoolComponent* pOutter);

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
  UCesiumBoundingVolumeComponent();
  virtual ~UCesiumBoundingVolumeComponent() = default;

  FPrimitiveSceneProxy* CreateSceneProxy() override;

  /**
   * Add an active view for this frame. Occlusion results for this bounding
   * volume will be pulled from the view and aggregated with the results from
   * other views. Call FinalizeOcclusionResultsForFrame after adding all the
   * relevant views.
   *
   * @param View An active view to retrieve occlusion results from.
   */
  void UpdateOcclusionFromView(const FSceneView* View);

  /**
   * Finalizes a collective occlusion result for this bounding volume from all
   * the views sent to UpdateOcclusionFromView.
   */
  void FinalizeOcclusionResultForFrame();

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

  bool IsMappedToTile() const { return this->_isMapped; }

  // virtual void BeginDestroy() override;

  // TileOcclusionRendererProxy implementation
  bool isOccluded() const override;

  bool isOcclusionAvailable() const override;

protected:
  void reset(const Cesium3DTilesSelection::Tile* pTile) override;

private:
  void _updateTransform();

  bool _isOccludedThisFrame = false;
  bool _isDefiniteThisFrame = true;
  bool _ignoreRemainingViews = false;

  bool _isOccluded = false;
  bool _isOcclusionAvailable = false;

  // Whether this proxy is currently mapped to a tile.
  bool _isMapped = false;

  Cesium3DTilesSelection::BoundingVolume _tileBounds =
      CesiumGeometry::OrientedBoundingBox(glm::dvec3(0.0), glm::dmat3(1.0));
  glm::dmat4 _tileTransform;
  glm::dmat4 _cesiumToUnreal;
};
