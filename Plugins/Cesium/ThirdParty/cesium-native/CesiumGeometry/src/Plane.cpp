#include "CesiumGeometry/Plane.h"
#include <glm/geometric.hpp>

namespace CesiumGeometry {

    Plane::Plane(const glm::dvec3& normal, double distance) :
        _normal(normal),
        _distance(distance)
    {
    }

    Plane::Plane(const glm::dvec3& point, const glm::dvec3& normal) :
        Plane(normal, -glm::dot(normal, point))
    {
    }

    double Plane::getPointDistance(const glm::dvec3& point) const {
        return glm::dot(this->_normal, point) + this->_distance;
    }

    glm::dvec3 Plane::projectPointOntoPlane(const glm::dvec3& point) const {
        // projectedPoint = point - (normal.point + scale) * normal
        double pointDistance = this->getPointDistance(point);
        glm::dvec3 scaledNormal = this->_normal * pointDistance;
        return point - scaledNormal;
    }

}
