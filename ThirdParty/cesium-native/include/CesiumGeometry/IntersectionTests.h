#pragma once

#include <optional>
#include <glm/vec3.hpp>

namespace Cesium3DTiles {
    class Ray;
    class Plane;

    class IntersectionTests {
    public:
        static std::optional<glm::dvec3> rayPlane(const Ray& ray, const Plane& plane);
    };

}
