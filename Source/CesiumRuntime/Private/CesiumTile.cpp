#include "CesiumTile.h"
#include "CalcBounds.h"
#include "CesiumTransforms.h"
#include "Components/PrimitiveComponent.h"
#include "VecMath.h"
#include <glm/gtc/matrix_inverse.hpp>

namespace {
struct OverlapComponentParameters {
  FVector Pos;
  FCollisionShape CollisionShape;
};

struct GetCollisionShape {
  const glm::dmat4& matrix;
  OverlapComponentParameters
  operator()(const CesiumGeometry::BoundingSphere& sphere) const {
    glm::dvec3 center =
        glm::dvec3(matrix * glm::dvec4(sphere.getCenter(), 1.0));
    return {
        VecMath::createVector(center),
        FCollisionShape::MakeSphere(sphere.getRadius() * 100.0)};
  }

  OverlapComponentParameters
  operator()(const CesiumGeometry::OrientedBoundingBox& box) const {
    glm::dvec3 center = glm::dvec3(matrix * glm::dvec4(box.getCenter(), 1.0));
    glm::dmat3 halfAxes = glm::dmat3(matrix) * box.getHalfAxes();
    double maxX = glm::abs(halfAxes[0].x) + glm::abs(halfAxes[1].x) +
                  glm::abs(halfAxes[2].x);
    double maxY = glm::abs(halfAxes[0].y) + glm::abs(halfAxes[1].y) +
                  glm::abs(halfAxes[2].y);
    double maxZ = glm::abs(halfAxes[0].z) + glm::abs(halfAxes[1].z) +
                  glm::abs(halfAxes[2].z);
    return {
        VecMath::createVector(center),
        FCollisionShape::MakeBox(FVector(maxX, maxY, maxZ))};
  }

  OverlapComponentParameters
  operator()(const CesiumGeospatial::BoundingRegion& region) const {
    return (*this)(region.getBoundingBox());
  }

  OverlapComponentParameters operator()(
      const CesiumGeospatial::BoundingRegionWithLooseFittingHeights& region)
      const {
    return (*this)(region.getBoundingRegion());
  }

  OverlapComponentParameters
  operator()(const CesiumGeospatial::S2CellBoundingVolume& s2) const {
    return (*this)(s2.computeBoundingRegion());
  }
};
} // namespace

FBoxSphereBounds UCesiumTile::GetBounds() const {
  return std::visit(
      CalcBoundsOperation{FTransform(), glm::affineInverse(_transform)},
      _pTile->getBoundingVolume());
}

bool UCesiumTile::IsIntersecting(const UPrimitiveComponent* Other) const {
  if (Other) {
    OverlapComponentParameters Parameters =
        std::visit(GetCollisionShape{_transform}, _pTile->getBoundingVolume());
    return Other->OverlapComponent(
        Parameters.Pos,
        FQuat::Identity,
        Parameters.CollisionShape);

  } else {
    return false;
  }
}

bool UCesiumTile::IsContainedBy(const UPrimitiveComponent* Other) const {
  if (Other) {
    FBox Box = GetBounds().GetBox();
    FBox OtherBox = Other->Bounds.GetBox();
    return OtherBox.IsInside(Box);
  } else {
    return false;
  }
}
