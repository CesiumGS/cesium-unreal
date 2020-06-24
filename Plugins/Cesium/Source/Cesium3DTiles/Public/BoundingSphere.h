#pragma once

#include <glm/vec3.hpp>

namespace Cesium3DTiles {

    class BoundingSphere {
    public:
        BoundingSphere() = default;

        BoundingSphere(const glm::dvec3& center, double radius) :
            center(center),
            radius(radius)
        {
        }

        glm::dvec3 center;
        double radius;
    };

}
