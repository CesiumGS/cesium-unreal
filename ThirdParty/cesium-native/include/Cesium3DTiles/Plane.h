#pragma once

#include <glm/vec3.hpp>

namespace Cesium3DTiles {

    class Plane {
    public:
        Plane() {}

        Plane(const glm::dvec3& normal, double distance) :
            normal(normal),
            distance(distance)
        {}
        
        glm::dvec3 normal;
        double distance;
    };
}
