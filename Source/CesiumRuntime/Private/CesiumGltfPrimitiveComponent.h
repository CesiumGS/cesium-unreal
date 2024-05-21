// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "Cesium3DTilesSelection/BoundingVolume.h"
#include "Cesium3DTileset.h"
#include "CesiumEncodedFeaturesMetadata.h"
#include "CesiumEncodedMetadataUtility.h"
#include "CesiumLifetime.h"
#include "CesiumMetadataPrimitive.h"
#include "CesiumPrimitiveFeatures.h"
#include "CesiumRasterOverlays.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PhysicsEngine/BodySetup.h"
#include "VecMath.h"
#include <CesiumGltf/AccessorUtility.h>
#include <cstdint>
#include <glm/mat4x4.hpp>
#include <unordered_map>
#include "CesiumGltfPrimitiveComponent.generated.h"

namespace CesiumGltf {
struct Model;
struct MeshPrimitive;
} // namespace CesiumGltf

class CesiumPrimitiveData {
public:
  CesiumPrimitiveData();

  /**
   * Represents the primitive's EXT_mesh_features extension.
   */
  FCesiumPrimitiveFeatures Features;
  /**
   * Represents the primitive's EXT_structural_metadata extension.
   */
  FCesiumPrimitiveMetadata Metadata;

  /**
   * The encoded representation of the primitive's EXT_mesh_features extension.
   */
  CesiumEncodedFeaturesMetadata::EncodedPrimitiveFeatures EncodedFeatures;
  /**
   * The encoded representation of the primitive's EXT_structural_metadata
   * extension.
   */
  CesiumEncodedFeaturesMetadata::EncodedPrimitiveMetadata EncodedMetadata;

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  /**
   * For backwards compatibility with the EXT_feature_metadata implementation.
   */
  FCesiumMetadataPrimitive Metadata_DEPRECATED;

  std::optional<CesiumEncodedMetadataUtility::EncodedMetadataPrimitive>
      EncodedMetadata_DEPRECATED;
  PRAGMA_ENABLE_DEPRECATION_WARNINGS

  ACesium3DTileset* pTilesetActor;
  const CesiumGltf::Model* pModel;
  const CesiumGltf::MeshPrimitive* pMeshPrimitive;

  /**
   * The double-precision transformation matrix for this glTF node.
   */
  glm::dmat4x4 HighPrecisionNodeTransform;

  /**
   * Maps an overlay texture coordinate ID to the index of the corresponding
   * texture coordinates in the mesh's UVs array.
   */
  OverlayTextureCoordinateIDMap overlayTextureCoordinateIDToUVIndex;

  /**
   * Maps the accessor index in a glTF to its corresponding texture coordinate
   * index in the Unreal mesh. The -1 key is reserved for implicit feature IDs
   * (in other words, the vertex index).
   */
  std::unordered_map<int32_t, uint32_t> GltfToUnrealTexCoordMap;

  /**
   * Maps texture coordinate set indices in a glTF to AccessorViews. This stores
   * accessor views on texture coordinate sets that will be used by feature ID
   * textures or property textures for picking.
   */
  std::unordered_map<int32_t, CesiumGltf::TexCoordAccessorType>
      TexCoordAccessorMap;

  /**
   * The position accessor of the glTF primitive. This is used for computing
   * the UV at a hit location on a primitive, and is safer to access than the
   * mesh's RenderData.
   */
  CesiumGltf::AccessorView<FVector3f> PositionAccessor;

  /**
   * The index accessor of the glTF primitive, if one is specified. This is used
   * for computing the UV at a hit location on a primitive.
   */
  CesiumGltf::IndexAccessorType IndexAccessor;

  std::optional<Cesium3DTilesSelection::BoundingVolume> boundingVolume;

  void BeginDestroyPrimitive();
};

template <typename UnrealType>
void UpdateTransformFromCesium(
    const glm::dmat4& CesiumToUnrealTransform,
    UnrealType* uobject,
    CesiumPrimitiveData* cesiumData) {
  const FTransform transform = FTransform(VecMath::createMatrix(
      CesiumToUnrealTransform * cesiumData->HighPrecisionNodeTransform));

  if (uobject->Mobility == EComponentMobility::Movable) {
    // For movable objects, move the component in the normal way, but don't
    // generate collisions along the way. Teleporting physics is imperfect,
    // but it's the best available option.
    uobject->SetRelativeTransform(
        transform,
        false,
        nullptr,
        ETeleportType::TeleportPhysics);
  } else {
    // Unreal will yell at us for calling SetRelativeTransform on a static
    // object, but we still need to adjust (accurately!) for origin rebasing
    // and georeference changes. It's "ok" to move a static object in this way
    // because, we assume, the globe and globe-oriented lights, etc. are
    // moving too, so in a relative sense the object isn't actually moving.
    // This isn't a perfect assumption, of course.
    uobject->SetRelativeTransform_Direct(transform);
    uobject->UpdateComponentToWorld();
    uobject->MarkRenderTransformDirty();
    uobject->SendPhysicsTransform(ETeleportType::ResetPhysics);
  }
}

template <typename UnrealType>
void BeginDestroyPrimitive(
    UnrealType* uobject,
    CesiumPrimitiveData* cesiumData) {
  // Clear everything we can in order to reduce memory usage, because this
  // UObject might not actually get deleted by the garbage collector until
  // much later.
  cesiumData->BeginDestroyPrimitive();
  UMaterialInstanceDynamic* pMaterial =
      Cast<UMaterialInstanceDynamic>(uobject->GetMaterial(0));
  if (pMaterial) {
    CesiumLifetime::destroy(pMaterial);
  }

  UStaticMesh* pMesh = uobject->GetStaticMesh();
  if (pMesh) {
    UBodySetup* pBodySetup = pMesh->GetBodySetup();

    if (pBodySetup) {
      CesiumLifetime::destroy(pBodySetup);
    }

    CesiumLifetime::destroy(pMesh);
  }
}

template <typename T>
class CesiumGltfPrimitive : public CesiumPrimitiveData {
public:
  T* _uobject;
};

UCLASS()
class UCesiumGltfPrimitiveComponent : public UStaticMeshComponent {
  GENERATED_BODY()

public:
  // Sets default values for this component's properties
  UCesiumGltfPrimitiveComponent();
  virtual ~UCesiumGltfPrimitiveComponent();

  virtual void BeginDestroy() override;

  virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const;

  using UPrimitiveComponent::SendPhysicsTransform;

  friend CesiumPrimitiveData* getPrimitiveData(USceneComponent* Component);
  friend const CesiumPrimitiveData*
  getPrimitiveData(const USceneComponent* Component);

protected:
  CesiumPrimitiveData _cesiumBase;
};

UCLASS()
class UCesiumGltfInstancedComponent : public UInstancedStaticMeshComponent {
  GENERATED_BODY()

public:
  // Sets default values for this component's properties
  UCesiumGltfInstancedComponent();
  virtual ~UCesiumGltfInstancedComponent();

  virtual void BeginDestroy() override;

  virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const;

  using UPrimitiveComponent::SendPhysicsTransform;

  friend CesiumPrimitiveData* getPrimitiveData(USceneComponent* Component);
  friend const CesiumPrimitiveData*
  getPrimitiveData(const USceneComponent* Component);

protected:
  CesiumPrimitiveData _cesiumBase;
};

CesiumPrimitiveData* getPrimitiveData(USceneComponent* pComponent);
const CesiumPrimitiveData*
getPrimitiveData(const USceneComponent* pComponent);
