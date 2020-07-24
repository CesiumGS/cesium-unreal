#pragma once

#include <glm/vec3.hpp>
#include "CesiumGeometry/Library.h"

namespace CesiumGeometry {

    class CESIUMGEOMETRY_API Plane {
    public:
        Plane(const glm::dvec3& normal, double distance);
        Plane(const glm::dvec3& point, const glm::dvec3& normal);

        const glm::dvec3& getNormal() const { return this->_normal; }
        double getDistance() const { return this->_distance; }

        double getPointDistance(const glm::dvec3& point) const;

        glm::dvec3 projectPointOntoPlane(const glm::dvec3& point) const;

    private:
        glm::dvec3 _normal;
        double _distance;
    };
}
