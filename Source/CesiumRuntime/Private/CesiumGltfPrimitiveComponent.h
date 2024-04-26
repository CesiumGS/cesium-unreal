// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "Cesium3DTilesSelection/BoundingVolume.h"
#include "Cesium3DTileset.h"
#include "CesiumEncodedFeaturesMetadata.h"
#include "CesiumEncodedMetadataUtility.h"
#include "CesiumMetadataPrimitive.h"
#include "CesiumPrimitiveFeatures.h"
#include "CesiumRasterOverlays.h"
#include "Components/StaticMeshComponent.h"
#include "CoreMinimal.h"
#include <CesiumGltf/AccessorUtility.h>
#include <cstdint>
#include <glm/mat4x4.hpp>
#include <unordered_map>
#include "CesiumGltfPrimitiveComponent.generated.h"

namespace CesiumGltf {
struct Model;
struct MeshPrimitive;
} // namespace CesiumGltf

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
