#pragma once

#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>
#include "CullingResult.h"

namespace Cesium3DTiles {

    class Plane;

    class BoundingBox {
    public:
        BoundingBox() = default;

        BoundingBox(
            const glm::dvec3& center,
            const glm::dmat3& halfAxes
        ) :
            center(center),
            halfAxes(halfAxes)
        {}

        CullingResult intersectPlane(const Plane& plane) const;

        glm::dvec3 center;
        glm::dmat3 halfAxes;
    };

}
