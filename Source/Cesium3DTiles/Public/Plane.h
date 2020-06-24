#pragma once

#include <glm/vec4.hpp>

namespace Cesium3DTiles {

    class Plane {
    public:
        Plane() {}

        Plane(const glm::vec4& coefficients) :
            coefficients(coefficients)
        {}
        
        glm::vec4 coefficients;
    };

}
