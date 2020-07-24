#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include "Cesium3DTiles/Ellipsoid.h"
#include "CesiumGeometry/Plane.h"

namespace Cesium3DTiles {

    class EllipsoidTangentPlane {
    public:
        EllipsoidTangentPlane(
            const glm::dvec3& origin,
            const Ellipsoid& ellipsoid = Ellipsoid::WGS84
        );

        EllipsoidTangentPlane(
            const glm::dmat4& eastNorthUpToFixedFrame,
            const Ellipsoid& ellipsoid = Ellipsoid::WGS84
        );

        const Ellipsoid& getEllipsoid() const { return this->_ellipsoid; }
        const glm::dvec3& getOrigin() const { return this->_origin; }
        const glm::dvec3& getXAxis() const { return this->_xAxis; }
        const glm::dvec3& getYAxis() const { return this->_yAxis; }
        const glm::dvec3& getZAxis() const { return this->_plane.getNormal(); }
        const Plane& getPlane() const { return this->_plane; }

        glm::dvec2 projectPointToNearestOnPlane(const glm::dvec3& cartesian);

    private:
        Ellipsoid _ellipsoid;
        glm::dvec3 _origin;
        glm::dvec3 _xAxis;
        glm::dvec3 _yAxis;
        Plane _plane;
    };

}
