// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Cesium3DTilesSelection/BoundingVolume.h"
#include "Cesium3DTileset.h"
#include "CesiumEncodedFeaturesMetadata.h"
#include "CesiumEncodedMetadataUtility.h"
#include "CesiumGltf/MeshPrimitive.h"
#include "CesiumGltf/Model.h"
#include "CesiumMetadataPrimitive.h"
#include "CesiumPrimitiveFeatures.h"
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

  OverlayTextureCoordinateIDMap overlayTextureCoordinateIDToUVIndex;
  // Maps the accessor index in a glTF to its corresponding texture coordinate
  // index in the Unreal mesh.
  // The -1 key is reserved for implicit feature IDs (in other words, the vertex
  // index).
  std::unordered_map<int32_t, uint32_t> textureCoordinateMap;

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
