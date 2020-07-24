#include "CesiumGeometry/BoundingSphere.h"
#include <glm/geometric.hpp>
#include "CesiumGeometry/Plane.h"

namespace CesiumGeometry {

    BoundingSphere::BoundingSphere(const glm::dvec3& center, double radius) :
        _center(center),
        _radius(radius)
    {
    }

    CullingResult BoundingSphere::intersectPlane(const Plane& plane) const {
        double distanceToPlane = glm::dot(plane.getNormal(), this->_center) + plane.getDistance();

        double radius = this->_radius;
        if (distanceToPlane < -radius) {
            // The center point is negative side of the plane normal
            return CullingResult::Outside;
        } else if (distanceToPlane < radius) {
            // The center point is positive side of the plane, but radius extends beyond it; partial overlap
            return CullingResult::Intersecting;
        }
        return CullingResult::Inside;
    }

    double BoundingSphere::computeDistanceSquaredToPosition(const glm::dvec3& position) const {
        glm::dvec3 diff = position - this->_center;
        return glm::dot(diff, diff) - this->_radius * this->_radius;
    }

}
