#pragma once

#include "CesiumGeospatial/Library.h"
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include "CesiumGeospatial/Ellipsoid.h"

namespace CesiumGeospatial {

    class CESIUMGEOSPATIAL_API Transforms {
    public:
        static glm::dmat4x4 eastNorthUpToFixedFrame(const glm::dvec3& origin, const Ellipsoid& ellipsoid = Ellipsoid::WGS84);
    };

}
