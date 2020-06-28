#pragma once

#include <glm/vec3.hpp>
#include "CullingResult.h"

namespace Cesium3DTiles {

    class Plane;

    class BoundingSphere {
    public:
        BoundingSphere() = default;

        BoundingSphere(const glm::dvec3& center, double radius) :
            center(center),
            radius(radius)
        {
        }
        
        CullingResult intersectPlane(const Plane& plane) const;

        glm::dvec3 center;
        double radius;
    };

}
