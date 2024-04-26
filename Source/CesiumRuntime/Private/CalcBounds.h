// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "Math/BoxSphereBounds.h"
#include "Math/TransformNonVectorized.h"
#include <Cesium3DTilesSelection/BoundingVolume.h>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>

struct CalcBoundsOperation {
  const FTransform& localToWorld;
  const glm::dmat4& highPrecisionTransform;

  // Bounding volumes are expressed in tileset coordinates, which is usually
  // ECEF.
  //
  // - `localToWorld` goes from model coordinates to Unreal world
  //    coordinates, where model coordinates include the tile's transform as
  //    well as any glTF node transforms.
  // - `highPrecisionTransform` transforms from model coordinates to tileset
  //    coordinates.
  //
  // So to transform a bounding volume, we need to first transform by the
  // inverse of `highPrecisionTransform` in order bring the bounding volume
  // into model coordinates, and then transform by `localToWorld` to bring the
  // bounding volume into Unreal world coordinates.

  glm::dmat4 getModelToUnrealWorldMatrix() const;

  glm::dmat4 getTilesetToUnrealWorldMatrix() const;

  FBoxSphereBounds
  operator()(const CesiumGeometry::BoundingSphere& sphere) const;

  FBoxSphereBounds
  operator()(const CesiumGeometry::OrientedBoundingBox& box) const;

  FBoxSphereBounds
  operator()(const CesiumGeospatial::BoundingRegion& region) const;

  FBoxSphereBounds operator()(
      const CesiumGeospatial::BoundingRegionWithLooseFittingHeights& region)
      const;

  FBoxSphereBounds
  operator()(const CesiumGeospatial::S2CellBoundingVolume& s2) const;
};