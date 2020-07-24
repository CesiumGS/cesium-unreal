#pragma once

#include <glm/vec3.hpp>
#include <optional>
#include "CesiumGeospatial/Cartographic.h"

namespace Cesium3DTiles {

    class CESIUM3DTILES_API Ellipsoid {
    public:
        static const Ellipsoid WGS84;

        Ellipsoid(double x, double y, double z);
        Ellipsoid(const glm::dvec3& radii);

        const glm::dvec3& getRadii() const { return this->_radii; }

        glm::dvec3 geodeticSurfaceNormal(const glm::dvec3& position) const;
        glm::dvec3 geodeticSurfaceNormal(const Cartographic& cartographic) const;
        glm::dvec3 cartographicToCartesian(const Cartographic& cartographic) const;
        std::optional<Cartographic> cartesianToCartographic(const glm::dvec3& cartesian) const;
        std::optional<glm::dvec3> scaleToGeodeticSurface(const glm::dvec3& cartesian) const;

    private:
        glm::dvec3 _radii;
        glm::dvec3 _radiiSquared;
        glm::dvec3 _oneOverRadii;
        glm::dvec3 _oneOverRadiiSquared;
        double _centerToleranceSquared;
    };

}
