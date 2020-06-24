#include "BoundingBox.h"
#include <glm/geometric.hpp>
#include "Plane.h"

namespace Cesium3DTiles {

    CullingResult BoundingBox::intersectPlane(const Plane& plane) const {
        glm::dvec3 normal = plane.getNormal();

        // plane is used as if it is its normal; the first three components are assumed to be normalized
        double radEffective =
            abs(
                normal.x * this->xAxisDirectionAndHalfLength.x +
                normal.y * this->xAxisDirectionAndHalfLength.y +
                normal.z * this->xAxisDirectionAndHalfLength.z
            ) +
            abs(
                normal.x * this->yAxisDirectionAndHalfLength.x +
                normal.y * this->yAxisDirectionAndHalfLength.y +
                normal.z * this->yAxisDirectionAndHalfLength.z
            ) +
            abs(
                normal.x * this->zAxisDirectionAndHalfLength.x +
                normal.y * this->zAxisDirectionAndHalfLength.y +
                normal.z * this->zAxisDirectionAndHalfLength.z
            );

        double distanceToPlane = ::glm::dot(normal, this->center) + plane.getDistance();

        if (distanceToPlane <= -radEffective) {
            // The entire box is on the negative side of the plane normal
            return CullingResult::Outside;
        } else if (distanceToPlane >= radEffective) {
            // The entire box is on the positive side of the plane normal
            return CullingResult::Inside;
        }
        return CullingResult::Intersecting;
    }

}
