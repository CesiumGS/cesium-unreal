// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "Cesium3DTileset.h"
#include "CesiumEncodedMetadataUtility.h"
#include "CesiumLoadedTile.h"
#include "CesiumMetadataPrimitive.h"
#include "CesiumPrimitiveFeatures.h"
#include "CesiumPrimitiveMetadata.h"
#include "CesiumRasterOverlays.h"
#include "EncodedFeaturesMetadata.h"

#include <CesiumGltf/AccessorUtility.h>
#include <cstdint>
#include <glm/mat4x4.hpp>
#include <optional>
#include <unordered_map>

#include "CesiumPrimitive.generated.h"

namespace CesiumGltf {
struct Model;
struct MeshPrimitive;
} // namespace CesiumGltf

/**
 * Data that is common between Cesium glTF component classes.
 */
class CesiumPrimitiveData {
public:
  /**
   * The factor by which the positions in the glTF primitive is scaled up when
   * the Unreal mesh is populated.
   *
   * We scale up the meshes because Chaos has a degenerate triangle epsilon test
   * in `TriangleMeshImplicitObject.cpp` that is almost laughably too eager.
   * Perhaps it would be fine if our meshes actually used units of centimeters
   * like UE, but they usually use meters instead. With a factor of 1.0, UE will
   * consider a right triangle that is slightly less than ~10cm on each side to
   * be degenerate.
   *
   * This value should be a power-of-two so the the scale affects only the
   * exponent of coordinate values, not the mantissa, in order to reduce the
   * chances of losing precision.
   */
  static constexpr double positionScaleFactor = 1024.0;

  /**
   * The scale matrix to apply to positions in the glTF primitive, derived from
   * positionScaleFactor.
   */
  static constexpr glm::dmat4 positionScaleMatrix = glm::dmat4(
      glm::dvec4(1.0 / positionScaleFactor, 0.0, 0.0, 0.0),
      glm::dvec4(0.0, 1.0 / positionScaleFactor, 0.0, 0.0),
      glm::dvec4(0.0, 0.0, 1.0 / positionScaleFactor, 0.0),
      glm::dvec4(0.0, 0.0, 0.0, 1.0));

  /**
   * A reference to the ACesium3DTileset that owns the primitive.
   */
  ACesium3DTileset* pTilesetActor = nullptr;

  /**
   * A reference to the glTF mesh primitive from which this was constructed.
   */
  const CesiumGltf::MeshPrimitive* pMeshPrimitive = nullptr;

  /**
   * Represents the primitive's EXT_mesh_features extension.
   */
  FCesiumPrimitiveFeatures features;
  /**
   * Represents the primitive's EXT_structural_metadata extension.
   */
  FCesiumPrimitiveMetadata metadata;

  /**
   * The encoded representation of the primitive's EXT_mesh_features extension.
   */
  EncodedFeaturesMetadata::EncodedPrimitiveFeatures encodedFeatures;
  /**
   * The encoded representation of the primitive's EXT_structural_metadata
   * extension.
   */
  EncodedFeaturesMetadata::EncodedPrimitiveMetadata encodedMetadata;

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  /**
   * For backwards compatibility with the EXT_feature_metadata implementation.
   */
  FCesiumMetadataPrimitive metadata_DEPRECATED;

  std::optional<CesiumEncodedMetadataUtility::EncodedMetadataPrimitive>
      encodedMetadata_DEPRECATED;
  PRAGMA_ENABLE_DEPRECATION_WARNINGS

  /**
   * The double-precision transformation matrix for this glTF node.
   */
  glm::dmat4x4 highPrecisionNodeTransform;

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
  std::unordered_map<int32_t, uint32_t> gltfToUnrealTexCoordMap;

  /**
   * Maps texture coordinate set indices in a glTF to AccessorViews. This stores
   * accessor views on texture coordinate sets that will be used by feature ID
   * textures or property textures for picking.
   */
  std::unordered_map<int32_t, CesiumGltf::TexCoordAccessorType>
      texCoordAccessorMap;

  /**
   * The position accessor of the glTF primitive. This is used for computing
   * the UV at a hit location on a primitive, and is safer to access than the
   * mesh's RenderData.
   */
  CesiumGltf::AccessorView<FVector3f> positionAccessor;

  /**
   * The index accessor of the glTF primitive, if one is specified. This is used
   * for computing the UV at a hit location on a primitive.
   */
  CesiumGltf::IndexAccessorType indexAccessor;

  /**
   * The bounding volume associated with the tile content to which this
   * primitive belongs.
   */
  std::optional<Cesium3DTilesSelection::BoundingVolume> boundingVolume;

  void destroy();
};

/**
 * Given a position from a glTF model, applies the scale factor and
 * Y-coordinate flip necessary for rendering in Unreal.
 */
FVector3f scalePositionForUnreal(const FVector3f& position);

UINTERFACE()
class UCesiumPrimitive : public UCesiumLoadedTilePrimitive {
  GENERATED_BODY()
};

/**
 * Common interface for data and functions used by Cesium mesh components.
 *
 * The rendering components used by Cesium may inherit from different classes in
 * the Unreal Component hierarchy, so a multiple inheritance interface approach
 * is used. Other ad-hoc functions are added to increase code reuse and make
 * certain functions (e.g., UpdateTransformFromCesium()) simpler.
 */
class ICesiumPrimitive : public ICesiumLoadedTilePrimitive {
  GENERATED_BODY()
public:
  virtual CesiumPrimitiveData& getPrimitiveData() = 0;
  virtual const CesiumPrimitiveData& getPrimitiveData() const = 0;

  // from ICesiumLoadedTilePrimitive:
  const CesiumGltf::MeshPrimitive* GetMeshPrimitive() const override;
  const FCesiumPrimitiveFeatures& GetPrimitiveFeatures() const override;
  const FCesiumPrimitiveMetadata& GetPrimitiveMetadata() const override;

  virtual void
  UpdateTransformFromCesium(const glm::dmat4& CesiumToUnrealTransform) = 0;
};
