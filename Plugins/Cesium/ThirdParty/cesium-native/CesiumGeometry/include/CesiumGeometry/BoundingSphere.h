#pragma once

#include <glm/vec3.hpp>
#include "CesiumGeometry/Library.h"
#include "CullingResult.h"

namespace CesiumGeometry {

    class Plane;

    class CESIUMGEOMETRY_API BoundingSphere {
    public:
        BoundingSphere(const glm::dvec3& center, double radius);

        const glm::dvec3& getCenter() const { return this-> _center; }
        double getRadius() const { return this->_radius; }
        
        CullingResult intersectPlane(const Plane& plane) const;
        double computeDistanceSquaredToPosition(const glm::dvec3& position) const;

    private:
        glm::dvec3 _center;
        double _radius;
    };

}
