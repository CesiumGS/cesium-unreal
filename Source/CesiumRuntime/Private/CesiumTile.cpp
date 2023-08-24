#include "CesiumTile.h"
#include "Components/PrimitiveComponent.h"
#include "VecMath.h"

namespace {
struct GetBoundsOperation {
  const glm::dmat4& matrix;
  FBoxSphereBounds
  operator()(const CesiumGeometry::BoundingSphere& sphere) const {
    glm::dvec3 center =
        glm::dvec3(matrix * glm::dvec4(sphere.getCenter(), 1.0));
    glm::dmat3 halfAxes = glm::dmat3(matrix) * glm::dmat3(sphere.getRadius());
    double sphereRadius =
        glm::max(glm::length(halfAxes[0]), glm::length(halfAxes[1]));
    sphereRadius = glm::max(sphereRadius, glm::length(halfAxes[2]));
    FBoxSphereBounds result;
    result.Origin = VecMath::createVector(center);
    result.SphereRadius = sphereRadius;
    result.BoxExtent = FVector(sphereRadius, sphereRadius, sphereRadius);
    return result;
  }

  FBoxSphereBounds
  operator()(const CesiumGeometry::OrientedBoundingBox& box) const {
    glm::dvec3 center = glm::dvec3(matrix * glm::dvec4(box.getCenter(), 1.0));
    glm::dmat3 halfAxes = glm::dmat3(matrix) * box.getHalfAxes();

    glm::dvec3 corner1 = halfAxes[0] + halfAxes[1];
    glm::dvec3 corner2 = halfAxes[0] + halfAxes[2];
    glm::dvec3 corner3 = halfAxes[1] + halfAxes[2];

    double sphereRadius = glm::max(glm::length(corner1), glm::length(corner2));
    sphereRadius = glm::max(sphereRadius, glm::length(corner3));

    double maxX = glm::abs(halfAxes[0].x) + glm::abs(halfAxes[1].x) +
                  glm::abs(halfAxes[2].x);
    double maxY = glm::abs(halfAxes[0].y) + glm::abs(halfAxes[1].y) +
                  glm::abs(halfAxes[2].y);
    double maxZ = glm::abs(halfAxes[0].z) + glm::abs(halfAxes[1].z) +
                  glm::abs(halfAxes[2].z);

    FBoxSphereBounds result;
    result.Origin = VecMath::createVector(center);
    result.SphereRadius = sphereRadius;
    result.BoxExtent = FVector(maxX, maxY, maxZ);
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

FBoxSphereBounds UCesiumTile::GetBounds() const {
  return std::visit(
      GetBoundsOperation{_transform},
      _pTile->getBoundingVolume());
}

bool UCesiumTile::IsIntersecting(UPrimitiveComponent* Other) const {
  if (Other) {
    FBoxSphereBounds Bounds = GetBounds();
    return FBoxSphereBounds::BoxesIntersect(Bounds, Other->Bounds) ||
           FBoxSphereBounds::SpheresIntersect(Bounds, Other->Bounds);
  } else {
    return false;
  }
}
