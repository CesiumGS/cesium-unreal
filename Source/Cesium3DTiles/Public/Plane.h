#pragma once

#include <glm/vec4.hpp>

namespace Cesium3DTiles {

    class Plane {
    public:
        Plane() {}

        Plane(const glm::dvec4& coefficients) :
            coefficients(coefficients)
        {}
        
        glm::dvec4 coefficients;

        glm::dvec3 getNormal() const { return glm::dvec3(this->coefficients); }
        double getDistance() const { return this->coefficients.w; }
    };

}
