#include "Cesium3DTiles/Ellipsoid.h"
#include <glm/geometric.hpp>

namespace Cesium3DTiles {

    /*static*/ const Ellipsoid Ellipsoid::WGS84 = Ellipsoid(6378137.0, 6378137.0, 6356752.3142451793);

    Ellipsoid::Ellipsoid(double x, double y, double z) :
        Ellipsoid(glm::dvec3(x, y, z))
    {
    }

    Ellipsoid::Ellipsoid(const glm::dvec3& radii) :
        _radii(radii),
        _radiiSquared(radii.x * radii.x, radii.y * radii.y, radii.z * radii.z)
    {
    }

    glm::dvec3 Ellipsoid::geodeticSurfaceNormal(const glm::dvec3& position) const {
        return glm::normalize(position * this->_radiiSquared);
    }
}
