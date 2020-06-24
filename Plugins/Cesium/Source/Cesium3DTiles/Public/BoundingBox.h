#pragma once

#include <glm/vec3.hpp>
#include "CullingResult.h"

namespace Cesium3DTiles {

    class Plane;

    class BoundingBox {
    public:
        BoundingBox() = default;

        BoundingBox(
            const glm::dvec3& center,
            const glm::dvec3& xAxisDirectionAndHalfLength,
            const glm::dvec3& yAxisDirectionAndHalfLength,
            const glm::dvec3& zAxisDirectionAndHalfLength
        ) :
            center(center),
            xAxisDirectionAndHalfLength(xAxisDirectionAndHalfLength),
            yAxisDirectionAndHalfLength(yAxisDirectionAndHalfLength),
            zAxisDirectionAndHalfLength(zAxisDirectionAndHalfLength)
        {}

        CullingResult intersectPlane(const Plane& plane) const;

        glm::dvec3 center;
        glm::dvec3 xAxisDirectionAndHalfLength;
        glm::dvec3 yAxisDirectionAndHalfLength;
        glm::dvec3 zAxisDirectionAndHalfLength;
    };

}
