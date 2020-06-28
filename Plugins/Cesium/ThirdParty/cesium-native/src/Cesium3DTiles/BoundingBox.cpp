#include "Cesium3DTiles/BoundingBox.h"
#include <glm/geometric.hpp>
#include "Cesium3DTiles/Plane.h"

namespace Cesium3DTiles {

    CullingResult BoundingBox::intersectPlane(const Plane& plane) const {
        glm::dvec3 normal = plane.normal;

        const glm::dvec3& xAxisDirectionAndHalfLength = this->halfAxes[0];
        const glm::dvec3& yAxisDirectionAndHalfLength = this->halfAxes[1];
        const glm::dvec3& zAxisDirectionAndHalfLength = this->halfAxes[2];

        // plane is used as if it is its normal; the first three components are assumed to be normalized
        double radEffective =
            abs(
                normal.x * xAxisDirectionAndHalfLength.x +
                normal.y * xAxisDirectionAndHalfLength.y +
                normal.z * xAxisDirectionAndHalfLength.z
            ) +
            abs(
                normal.x * yAxisDirectionAndHalfLength.x +
                normal.y * yAxisDirectionAndHalfLength.y +
                normal.z * yAxisDirectionAndHalfLength.z
            ) +
            abs(
                normal.x * zAxisDirectionAndHalfLength.x +
                normal.y * zAxisDirectionAndHalfLength.y +
                normal.z * zAxisDirectionAndHalfLength.z
            );

        double distanceToPlane = ::glm::dot(normal, this->center) + plane.distance;

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
