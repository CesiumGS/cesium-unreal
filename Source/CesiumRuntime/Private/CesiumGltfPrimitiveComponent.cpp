// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumGltfPrimitiveComponent.h"
#include "CesiumLifetime.h"
#include "CesiumMaterialUserData.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PhysicsEngine/BodySetup.h"
#include "VecMath.h"
#include <glm/gtc/matrix_inverse.hpp>

class FCesiumGltfPrimitiveSceneProxy : public FStaticMeshSceneProxy {
public:
  FCesiumGltfPrimitiveSceneProxy(const UCesiumGltfPrimitiveComponent* pComponent) :
    FStaticMeshSceneProxy((UStaticMeshComponent*)pComponent, false) {}

  // Explicitly disable dynamic occlusion culling on gltf primitives since we
  // will be handling it ourselves. The selection will receive occlusion 
  // feedback via a stand-in UCesiumBoundingVolumeComponent for every traversed
  // tile, so occluded tiles should automatically be culled. Unreal will waste
  // draw calls computing occlusion on these primitives if we don't disable it
  // here.
  bool CanBeOccluded() const override {
    return false;
  }
};

// Sets default values for this component's properties
UCesiumGltfPrimitiveComponent::UCesiumGltfPrimitiveComponent() {
  // Set this component to be initialized when the game starts, and to be ticked
  // every frame.  You can turn these features off to improve performance if you
  // don't need them.
  PrimaryComponentTick.bCanEverTick = false;
  pModel = nullptr;
  pMeshPrimitive = nullptr;
}

UCesiumGltfPrimitiveComponent::~UCesiumGltfPrimitiveComponent() {}

void UCesiumGltfPrimitiveComponent::UpdateTransformFromCesium(
    const glm::dmat4& CesiumToUnrealTransform) {
  this->SetUsingAbsoluteLocation(true);
  this->SetUsingAbsoluteRotation(true);
  this->SetUsingAbsoluteScale(true);

  const glm::dmat4x4& transform =
      CesiumToUnrealTransform * this->HighPrecisionNodeTransform;

  this->SetRelativeTransform(FTransform(FMatrix(
      FVector(transform[0].x, transform[0].y, transform[0].z),
      FVector(transform[1].x, transform[1].y, transform[1].z),
      FVector(transform[2].x, transform[2].y, transform[2].z),
      FVector(transform[3].x, transform[3].y, transform[3].z))));
}

FPrimitiveSceneProxy* UCesiumGltfPrimitiveComponent::CreateSceneProxy() {
  // Copied and adapted from UStaticMeshComponent::CreateSceneProxy

  if (GetStaticMesh() == nullptr || GetStaticMesh()->GetRenderData() == nullptr)
	{
		return nullptr;
	}

	const FStaticMeshLODResourcesArray& LODResources = GetStaticMesh()->GetRenderData()->LODResources;
	if (LODResources.Num() == 0	|| LODResources[FMath::Clamp<int32>(GetStaticMesh()->GetMinLOD().Default, 0, LODResources.Num()-1)].VertexBuffers.StaticMeshVertexBuffer.GetNumVertices() == 0)
	{
		return nullptr;
	}
	LLM_SCOPE(ELLMTag::StaticMesh);

	FPrimitiveSceneProxy* Proxy = new FCesiumGltfPrimitiveSceneProxy(this);

  //FPrimitiveSceneProxy* Proxy = new FStaticMeshSceneProxy(this);
#if STATICMESH_ENABLE_DEBUG_RENDERING
	SendRenderDebugPhysics(Proxy);
#endif

	return Proxy;
}

namespace {

void destroyMaterialTexture(
    UMaterialInstanceDynamic* pMaterial,
    const char* name,
    EMaterialParameterAssociation assocation,
    int32 index) {
  UTexture* pTexture = nullptr;
  if (pMaterial->GetTextureParameterValue(
          FMaterialParameterInfo(name, assocation, index),
          pTexture,
          true)) {
    CesiumLifetime::destroy(pTexture);
  }
}

void destroyGltfParameterValues(
    UMaterialInstanceDynamic* pMaterial,
    EMaterialParameterAssociation assocation,
    int32 index) {
  destroyMaterialTexture(pMaterial, "baseColorTexture", assocation, index);
  destroyMaterialTexture(
      pMaterial,
      "metallicRoughnessTexture",
      assocation,
      index);
  destroyMaterialTexture(pMaterial, "normalTexture", assocation, index);
  destroyMaterialTexture(pMaterial, "emissiveTexture", assocation, index);
  destroyMaterialTexture(pMaterial, "occlusionTexture", assocation, index);
}

void destroyWaterParameterValues(
    UMaterialInstanceDynamic* pMaterial,
    EMaterialParameterAssociation assocation,
    int32 index) {
  destroyMaterialTexture(pMaterial, "WaterMask", assocation, index);
}
} // namespace

void UCesiumGltfPrimitiveComponent::BeginDestroy() {
  // This should mirror the logic in loadPrimitiveGameThreadPart in
  // CesiumGltfComponent.cpp
  UMaterialInstanceDynamic* pMaterial =
      Cast<UMaterialInstanceDynamic>(this->GetMaterial(0));
  if (pMaterial) {

    destroyGltfParameterValues(
        pMaterial,
        EMaterialParameterAssociation::GlobalParameter,
        INDEX_NONE);
    destroyWaterParameterValues(
        pMaterial,
        EMaterialParameterAssociation::GlobalParameter,
        INDEX_NONE);

    UMaterialInterface* pBaseMaterial = pMaterial->Parent;
    UMaterialInstance* pBaseAsMaterialInstance =
        Cast<UMaterialInstance>(pBaseMaterial);
    UCesiumMaterialUserData* pCesiumData =
        pBaseAsMaterialInstance
            ? pBaseAsMaterialInstance
                  ->GetAssetUserData<UCesiumMaterialUserData>()
            : nullptr;
    if (pCesiumData) {
      destroyGltfParameterValues(
          pMaterial,
          EMaterialParameterAssociation::LayerParameter,
          0);

      int32 waterIndex = pCesiumData->LayerNames.Find("Water");
      if (waterIndex >= 0) {
        destroyWaterParameterValues(
            pMaterial,
            EMaterialParameterAssociation::LayerParameter,
            waterIndex);
      }
    }

    CesiumEncodedMetadataUtility::destroyEncodedMetadataPrimitive(
        this->EncodedMetadata);

    CesiumLifetime::destroy(pMaterial);
  }

  UStaticMesh* pMesh = this->GetStaticMesh();
  if (pMesh) {
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 27
    UBodySetup* pBodySetup = pMesh->BodySetup;
#else
    UBodySetup* pBodySetup = pMesh->GetBodySetup();
#endif
    if (pBodySetup) {
      CesiumLifetime::destroy(pBodySetup);
    }

    CesiumLifetime::destroy(pMesh);
  }

  Super::BeginDestroy();
}

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

FBoxSphereBounds UCesiumGltfPrimitiveComponent::CalcBounds(
    const FTransform& LocalToWorld) const {
  if (!this->boundingVolume) {
    return Super::CalcBounds(LocalToWorld);
  }

  return std::visit(
      CalcBoundsOperation{
          *this->boundingVolume,
          LocalToWorld,
          this->HighPrecisionNodeTransform},
      *this->boundingVolume);
}
