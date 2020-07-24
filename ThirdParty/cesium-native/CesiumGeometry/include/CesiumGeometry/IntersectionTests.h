#pragma once

#include <optional>
#include <glm/vec3.hpp>
#include "CesiumGeometry/Library.h"

namespace CesiumGeometry {
    class Ray;
    class Plane;

    class CESIUMGEOMETRY_API IntersectionTests {
    public:
        static std::optional<glm::dvec3> rayPlane(const Ray& ray, const Plane& plane);
    };

}
