#pragma once

#include <glm/vec3.hpp>

namespace Cesium3DTiles {

    class CESIUM3DTILES_API Ellipsoid {
    public:
        static const Ellipsoid WGS84;

        Ellipsoid(double x, double y, double z);
        Ellipsoid(const glm::dvec3& radii);

        const glm::dvec3& getRadii() const { return this->_radii; }

        glm::dvec3 geodeticSurfaceNormal(const glm::dvec3& position) const;

    private:
        glm::dvec3 _radii;
        glm::dvec3 _radiiSquared;
    };

}
