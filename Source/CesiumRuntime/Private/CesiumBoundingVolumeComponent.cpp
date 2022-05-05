// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumBoundingVolumeComponent.h"
#include "CesiumGeoreference.h"
#include "VecMath.h"
#include "UObject/UObjectGlobals.h"
#include <optional>
#include <variant>

using namespace Cesium3DTilesSelection;

UCesiumBoundingVolumePoolComponent::UCesiumBoundingVolumePoolComponent() {
  this->_pPool = 
      std::make_shared<UCesiumBoundingVolumePool>(this);
}

TileOcclusionRendererProxy* 
UCesiumBoundingVolumePoolComponent::createProxy() {
  UCesiumBoundingVolumeComponent* pBoundingVolume = 
      NewObject<UCesiumBoundingVolumeComponent>(this);
  pBoundingVolume->SetVisibility(false);
  pBoundingVolume->bUseAsOccluder = false;
  pBoundingVolume->SetupAttachment(this);
  pBoundingVolume->RegisterComponent();
  return (TileOcclusionRendererProxy*)pBoundingVolume;
}

void 
UCesiumBoundingVolumePoolComponent::destroyProxy(
    TileOcclusionRendererProxy* pProxy) {
  // TODO: verify correctness of destruction
  UCesiumBoundingVolumeComponent* pBoundingVolumeComponent =
      (UCesiumBoundingVolumeComponent*)pProxy;
  if (pBoundingVolumeComponent) {
    pBoundingVolumeComponent->ConditionalBeginDestroy(); 
  }
}

UCesiumBoundingVolumePool::UCesiumBoundingVolumePool(UCesiumBoundingVolumePoolComponent* pOutter) :
    _pOutter(pOutter) {}

TileOcclusionRendererProxy* 
UCesiumBoundingVolumePool::createProxy() {
  return this->_pOutter->createProxy();
}

void 
UCesiumBoundingVolumePool::destroyProxy(
    TileOcclusionRendererProxy* pProxy) {
  this->_pOutter->destroyProxy(pProxy);
}

void UCesiumBoundingVolumePoolComponent::UpdateTransformFromCesium(
    const glm::dmat4& CesiumToUnrealTransform) {
  TArray<USceneComponent*> children = this->GetAttachChildren();
  for (USceneComponent* pChild : children) {
    UCesiumBoundingVolumeComponent* pBoundingVolume = 
        Cast<UCesiumBoundingVolumeComponent>(pChild);
    if (pBoundingVolume) {
      pBoundingVolume->UpdateTransformFromCesium(CesiumToUnrealTransform);
    }
  }
}

class FCesiumBoundingVolumeSceneProxy : public FPrimitiveSceneProxy {

public:
  FCesiumBoundingVolumeSceneProxy(
    const UCesiumBoundingVolumeComponent* pComponent,
    const BoundingVolume& localTileBounds) :
      FPrimitiveSceneProxy(pComponent/*, name?*/),
      _localTileBounds(localTileBounds) {}

  //bool HasCustomOcclusionBounds() const override {
  //  return true;
  //}

  //FBoxSphereBounds GetCustomOcclusionBounds() const override {
    // TODO: fix this

    // TODO: find better default?
    //return FBoxSphereBounds();
  //}

  void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override {
    for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++) {
      /* * / RenderBounds(
          Collector.GetPDI(ViewIndex),
          ViewFamily.EngineShowFlags,
          GetBounds(),
          true /*!Owner || IsSelected()* /);*/

		  const ESceneDepthPriorityGroup DrawBoundsDPG = SDPG_World;
		  DrawWireBox(
          Collector.GetPDI(ViewIndex),
          GetBounds().GetBox(),
          FColor(72, 72, 255),
          DrawBoundsDPG);
    }
  }
  
  FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override {
    FPrimitiveViewRelevance Result;
    Result.bDrawRelevance = true;
    Result.bDynamicRelevance = true;
    // Result.bEditorPrimitiveRelevance = true;
    return Result;
  }

  // TODO: IMPLEMENT THIS
  // this is almost certainly the key to removing _all_ current hacks 
  void AcceptOcclusionResults(
      const FSceneView* View, 
      TArray<bool>* Results, 
      int32 ResultsStart, 
      int32 NumResults) override {
    
  }

  /*/
  HHitProxy* CreateHitProxies(
      UPrimitiveComponent* Component,
      TArray<TRefCountPtr<HHitProxy>>& OutHitProxies) override {
    return nullptr;
  }*/

  SIZE_T GetTypeHash() const override {
    static size_t UniquePointer;
    return reinterpret_cast<size_t>(&UniquePointer);
  }

  uint32 GetMemoryFootprint(void) const override {
    return 0;
  }

private:
  BoundingVolume _localTileBounds;
};

FPrimitiveSceneProxy* UCesiumBoundingVolumeComponent::CreateSceneProxy() {
  return 
      (FPrimitiveSceneProxy*)new FCesiumBoundingVolumeSceneProxy(
        this, 
        this->_localTileBounds);
}

UCesiumBoundingVolumeComponent::UCesiumBoundingVolumeComponent() 
    : _localTileBounds(CesiumGeometry::OrientedBoundingBox(
          glm::dvec3(0.0), glm::dmat3(1.0))),
      _tileTransform(1.0),
      _cesiumToUnreal(1.0) {}

void UCesiumBoundingVolumeComponent::SetOcclusionResult_RenderThread(
    const std::optional<bool>& isOccluded) {
  this->_isOccluded_RenderThread = isOccluded;
}

void UCesiumBoundingVolumeComponent::SyncOcclusionResult() {
  if (this->_isOccluded_RenderThread) {
    this->_isOccluded = *this->_isOccluded_RenderThread;
    this->_occlusionUpdatedThisFrame = true;
    this->_isOccluded_RenderThread = std::nullopt;
  }
}

void UCesiumBoundingVolumeComponent::_updateTransform() {
  this->SetUsingAbsoluteLocation(true);
  this->SetUsingAbsoluteRotation(true);
  this->SetUsingAbsoluteScale(true);

  const glm::dmat4x4& transform =
      this->_cesiumToUnreal * this->_tileTransform;

  this->SetRelativeTransform(FTransform(FMatrix(
      FVector(transform[0].x, transform[0].y, transform[0].z),
      FVector(transform[1].x, transform[1].y, transform[1].z),
      FVector(transform[2].x, transform[2].y, transform[2].z),
      FVector(transform[3].x, transform[3].y, transform[3].z))));
}

void UCesiumBoundingVolumeComponent::UpdateTransformFromCesium(
    const glm::dmat4& CesiumToUnrealTransform) {
  this->_cesiumToUnreal = CesiumToUnrealTransform;
  this->_updateTransform();
}

bool UCesiumBoundingVolumeComponent::isOccluded() const {
  return this->_isOccluded;
}

int32_t UCesiumBoundingVolumeComponent::getLastUpdatedFrame() const {
  return this->_lastUpdatedFrame;
}

void UCesiumBoundingVolumeComponent::reset(const Tile* pTile) {
  if (pTile) {
    // TODO: reject incompatible bounding volume types? Convert them all to
    // OBB / spheres?
    this->_tileTransform = pTile->getTransform();
    // TODO: content BV instead? 
    // TODO: union of children BVs instead?
    this->_localTileBounds = pTile->getBoundingVolume();
    //    transformBoundingVolume(
    //      glm::inverse(this->_tileTransform),
    //     pTile->getBoundingVolume());
    this->_updateTransform();

    this->SetVisibility(true);
    // TODO: might have to mark render state dirty somewhere here
  } else {
    this->_isOccluded = false;
    this->_lastUpdatedFrame = -1000;
    this->SetVisibility(false);
  }
}

void UCesiumBoundingVolumeComponent::update(int32_t currentFrame) {
  if (this->_occlusionUpdatedThisFrame) {
    this->_lastUpdatedFrame = currentFrame;
    this->_occlusionUpdatedThisFrame = false;
  }
}

// TEMP COPY MOVE TO UTIL
namespace {
struct CalcBoundsOperation {
  const Cesium3DTilesSelection::BoundingVolume& boundingVolume;
  const FTransform& localToWorld;
  const glm::dmat4& highPrecisionNodeTransform;

  // Bounding volumes are expressed in tileset coordinates, which is usually
  // ECEF.
  //
  // - `localToWorld` goes from model coordinates to Unreal world
  //    coordinates, where model coordinates include the tile's transform as
  //    well as any glTF node transforms.
  // - `highPrecisionNodeTransform` transforms from model coordinates to tileset
  //    coordinates.
  //
  // So to transform a bounding volume, we need to first transform by the
  // inverse of `HighPrecisionNodeTransform` in order bring the bounding volume
  // into model coordinates, and then transform by `localToWorld` to bring the
  // bounding volume into Unreal world coordinates.

  glm::dmat4 getModelToUnrealWorldMatrix() const {
    const FMatrix matrix = localToWorld.ToMatrixWithScale();
    return VecMath::createMatrix4D(matrix);
  }

  glm::dmat4 getTilesetToUnrealWorldMatrix() const {
    const glm::dmat4 modelToUnreal = getModelToUnrealWorldMatrix();
    const glm::dmat4 tilesetToModel =
        glm::affineInverse(highPrecisionNodeTransform);
    return modelToUnreal * tilesetToModel;
  }

  FBoxSphereBounds
  operator()(const CesiumGeometry::BoundingSphere& sphere) const {
    glm::dmat4 matrix = getTilesetToUnrealWorldMatrix();
    glm::dvec3 center =
        glm::dvec3(matrix * glm::dvec4(sphere.getCenter(), 1.0));
    glm::dmat3 halfAxes = glm::dmat3(matrix) * glm::dmat3(sphere.getRadius());

    // The sphere only needs to reach the sides of the box, not the corners.
    double sphereRadius =
        glm::max(glm::length(halfAxes[0]), glm::length(halfAxes[1]));
    sphereRadius = glm::max(sphereRadius, glm::length(halfAxes[2]));

    FVector xs(halfAxes[0].x, halfAxes[1].x, halfAxes[2].x);
    FVector ys(halfAxes[0].y, halfAxes[1].y, halfAxes[2].y);
    FVector zs(halfAxes[0].z, halfAxes[1].z, halfAxes[2].z);

    FBoxSphereBounds result;
    result.Origin = VecMath::createVector(center);
    result.SphereRadius = sphereRadius;
    result.BoxExtent = FVector(xs.GetAbsMax(), ys.GetAbsMax(), zs.GetAbsMax());
    return result;
  }

  FBoxSphereBounds
  operator()(const CesiumGeometry::OrientedBoundingBox& box) const {
    glm::dmat4 matrix = getTilesetToUnrealWorldMatrix();
    glm::dvec3 center = glm::dvec3(matrix * glm::dvec4(box.getCenter(), 1.0));
    glm::dmat3 halfAxes = glm::dmat3(matrix) * box.getHalfAxes();

    glm::dvec3 corner1 = halfAxes[0] + halfAxes[1];
    glm::dvec3 corner2 = halfAxes[0] + halfAxes[2];
    glm::dvec3 corner3 = halfAxes[1] + halfAxes[2];

    double sphereRadius = glm::max(glm::length(corner1), glm::length(corner2));
    sphereRadius = glm::max(sphereRadius, glm::length(corner3));

    FVector xs(halfAxes[0].x, halfAxes[1].x, halfAxes[2].x);
    FVector ys(halfAxes[0].y, halfAxes[1].y, halfAxes[2].y);
    FVector zs(halfAxes[0].z, halfAxes[1].z, halfAxes[2].z);

    FBoxSphereBounds result;
    result.Origin = VecMath::createVector(center);
    result.SphereRadius = sphereRadius;
    result.BoxExtent = FVector(xs.GetAbsMax(), ys.GetAbsMax(), zs.GetAbsMax());
    return result;
  }

  FBoxSphereBounds
  operator()(const CesiumGeospatial::BoundingRegion& region) const {
    return (*this)(region.getBoundingBox());
  }

  FBoxSphereBounds operator()(
      const CesiumGeospatial::BoundingRegionWithLooseFittingHeights& region)
      const {
    return (*this)(region.getBoundingRegion());
  }

  FBoxSphereBounds
  operator()(const CesiumGeospatial::S2CellBoundingVolume& s2) const {
    return (*this)(s2.computeBoundingRegion());
  }
};

} // namespace

FBoxSphereBounds UCesiumBoundingVolumeComponent::CalcBounds(
    const FTransform& LocalToWorld) const {
  //if (!this->boundingVolume) {
  //  return Super::CalcBounds(LocalToWorld);
  //}

  return std::visit(
      CalcBoundsOperation{
          this->_localTileBounds,
          LocalToWorld,
          this->_tileTransform},
      this->_localTileBounds);
}
