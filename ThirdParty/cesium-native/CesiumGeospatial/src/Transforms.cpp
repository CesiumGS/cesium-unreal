#include "CesiumGeospatial/Transforms.h"
#include <glm/gtc/epsilon.hpp>
#include "CesiumUtility/Math.h"

using namespace CesiumUtility;

namespace CesiumGeospatial {

/*static*/ glm::dmat4x4 Transforms::eastNorthUpToFixedFrame(const glm::dvec3& origin, const Ellipsoid& ellipsoid /*= Ellipsoid::WGS84*/) {
    if (Math::equalsEpsilon(origin, glm::dvec3(0.0), Math::EPSILON14)) {
        // If x, y, and z are zero, use the degenerate local frame, which is a special case
        return glm::dmat4x4(
            glm::dvec4(0.0, 1.0, 0.0, 0.0),
            glm::dvec4(-1.0, 0.0, 0.0, 0.0),
            glm::dvec4(0.0, 0.0, 1.0, 0.0),
            glm::dvec4(origin, 1.0)
        );
    } else if (Math::equalsEpsilon(origin.x, 0.0, Math::EPSILON14) && Math::equalsEpsilon(origin.y, 0.0, Math::EPSILON14)) {
        // If x and y are zero, assume origin is at a pole, which is a special case.
        double sign = Math::sign(origin.z);
        return glm::dmat4x4(
            glm::dvec4(0.0, 1.0, 0.0, 0.0),
            glm::dvec4(-1.0 * sign, 0.0, 0.0, 0.0),
            glm::dvec4(0.0, 0.0, 1.0 * sign, 0.0),
            glm::dvec4(origin, 1.0)
        );
    } else {
        glm::dvec3 up = ellipsoid.geodeticSurfaceNormal(origin);
        glm::dvec3 east = glm::normalize(glm::dvec3(-origin.y, origin.x, 0.0));
        glm::dvec3 north = glm::cross(up, east);

        return glm::dmat4x4(
            glm::dvec4(east, 0.0),
            glm::dvec4(north, 0.0),
            glm::dvec4(up, 0.0),
            glm::dvec4(origin, 1.0)
        );
    }
}

}
