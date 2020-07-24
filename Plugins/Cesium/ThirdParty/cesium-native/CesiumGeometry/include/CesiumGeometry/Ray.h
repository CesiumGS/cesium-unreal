#pragma once

#include <glm/vec3.hpp>

namespace Cesium3DTiles {

    class Ray {
    public:
        Ray(const glm::dvec3& origin, const glm::dvec3& direction);

        const glm::dvec3& getOrigin() const { return this->_origin; }
        const glm::dvec3& getDirection() const { return this->_direction; }

        Ray operator-() const;

    private:
        glm::dvec3 _origin;
        glm::dvec3 _direction;
    };
}
