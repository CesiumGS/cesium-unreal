// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Cesium3DTilesSelection/BoundingVolume.h"
#include "CesiumEncodedMetadataUtility.h"
#include "CesiumGltf/MeshPrimitive.h"
#include "CesiumGltf/Model.h"
#include "CesiumMetadataPrimitive.h"
#include "CesiumRasterOverlays.h"
#include "Components/StaticMeshComponent.h"
#include "CoreMinimal.h"
#include <cstdint>
#include <glm/mat4x4.hpp>
#include <unordered_map>
#include "CesiumGltfPrimitiveComponent.generated.h"

UCLASS()
class UCesiumGltfPrimitiveComponent : public UStaticMeshComponent {
  GENERATED_BODY()

public:
  // Sets default values for this component's properties
  UCesiumGltfPrimitiveComponent();
  virtual ~UCesiumGltfPrimitiveComponent();

  FCesiumMetadataPrimitive Metadata;

  CesiumEncodedMetadataUtility::EncodedMetadataPrimitive EncodedMetadata;

  const CesiumGltf::Model* pModel;

  const CesiumGltf::MeshPrimitive* pMeshPrimitive;

  /**
   * The double-precision transformation matrix for this glTF node.
   */
  glm::dmat4x4 HighPrecisionNodeTransform;

  OverlayTextureCoordinateIDMap overlayTextureCoordinateIDToUVIndex;
  std::unordered_map<uint32_t, uint32_t> textureCoordinateMap;

  std::optional<Cesium3DTilesSelection::BoundingVolume> boundingVolume;

  /**
   * Updates this component's transform from a new double-precision
   * transformation from the Cesium world to the Unreal Engine world, as well as
   * the current HighPrecisionNodeTransform.
   *
   * @param CesiumToUnrealTransform The new transformation.
   */
  void UpdateTransformFromCesium(const glm::dmat4& CesiumToUnrealTransform);

  virtual void BeginDestroy() override;

  virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const;
};
