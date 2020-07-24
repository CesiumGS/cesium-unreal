#include "CesiumGeospatial/Ellipsoid.h"
#include <glm/geometric.hpp>
#include "CesiumUtility/Math.h"

namespace Cesium3DTiles {

    /*static*/ const Ellipsoid Ellipsoid::WGS84 = Ellipsoid(6378137.0, 6378137.0, 6356752.3142451793);

    Ellipsoid::Ellipsoid(double x, double y, double z) :
        Ellipsoid(glm::dvec3(x, y, z))
    {
    }

    Ellipsoid::Ellipsoid(const glm::dvec3& radii) :
        _radii(radii),
        _radiiSquared(radii.x * radii.x, radii.y * radii.y, radii.z * radii.z),
        _oneOverRadii(1.0 / radii.x, 1.0 / radii.y, 1.0 / radii.z),
        _oneOverRadiiSquared(1.0 / (radii.x * radii.x), 1.0 / (radii.y * radii.y), 1.0 / (radii.z * radii.z)),
        _centerToleranceSquared(Math::EPSILON1)
    {
    }

    glm::dvec3 Ellipsoid::geodeticSurfaceNormal(const glm::dvec3& position) const {
        return glm::normalize(position * this->_radiiSquared);
    }

    glm::dvec3 Ellipsoid::geodeticSurfaceNormal(const Cartographic& cartographic) const {
        double longitude = cartographic.longitude;
        double latitude = cartographic.latitude;
        double cosLatitude = cos(latitude);

        return glm::normalize(glm::dvec3(
            cosLatitude * cos(longitude),
            cosLatitude * sin(longitude),
            sin(latitude)
        ));
    }

    glm::dvec3 Ellipsoid::cartographicToCartesian(const Cartographic& cartographic) const {
        glm::dvec3 n = this->geodeticSurfaceNormal(cartographic);
        glm::dvec3 k = this->_radiiSquared * n;
        double gamma = sqrt(glm::dot(n, k));
        k /= gamma;
        n *= cartographic.height;
        return k + n;
    }

    std::optional<Cartographic> Ellipsoid::cartesianToCartographic(const glm::dvec3& cartesian) const {
        std::optional<glm::dvec3> p = this->scaleToGeodeticSurface(cartesian);

        if (!p) {
            return std::optional<Cartographic>();
        }

        glm::dvec3 n = this->geodeticSurfaceNormal(p.value());
        glm::dvec3 h = cartesian - p.value();

        double longitude = atan2(n.y, n.x);
        double latitude = asin(n.z);
        double height = Math::sign(glm::dot(h, cartesian)) * glm::length(h);

        return Cartographic(longitude, latitude, height);
    }

    std::optional<glm::dvec3> Ellipsoid::scaleToGeodeticSurface(const glm::dvec3& cartesian) const {
        double positionX = cartesian.x;
        double positionY = cartesian.y;
        double positionZ = cartesian.z;

        double oneOverRadiiX = this->_oneOverRadii.x;
        double oneOverRadiiY = this->_oneOverRadii.y;
        double oneOverRadiiZ = this->_oneOverRadii.z;

        double x2 = positionX * positionX * oneOverRadiiX * oneOverRadiiX;
        double y2 = positionY * positionY * oneOverRadiiY * oneOverRadiiY;
        double z2 = positionZ * positionZ * oneOverRadiiZ * oneOverRadiiZ;

        // Compute the squared ellipsoid norm.
        double squaredNorm = x2 + y2 + z2;
        double ratio = sqrt(1.0 / squaredNorm);

        // As an initial approximation, assume that the radial intersection is the projection point.
        glm::dvec3 intersection = cartesian * ratio;

        // If the position is near the center, the iteration will not converge.
        if (squaredNorm < this->_centerToleranceSquared) {
            return !std::isfinite(ratio)
                ? std::optional<glm::dvec3>()
                : intersection;
        }

        double oneOverRadiiSquaredX = this->_oneOverRadiiSquared.x;
        double oneOverRadiiSquaredY = this->_oneOverRadiiSquared.y;
        double oneOverRadiiSquaredZ = this->_oneOverRadiiSquared.z;

        // Use the gradient at the intersection point in place of the true unit normal.
        // The difference in magnitude will be absorbed in the multiplier.
        glm::dvec3 gradient(
            intersection.x * oneOverRadiiSquaredX * 2.0,
            intersection.y * oneOverRadiiSquaredY * 2.0,
            intersection.z * oneOverRadiiSquaredZ * 2.0
        );

        // Compute the initial guess at the normal vector multiplier, lambda.
        double lambda =
            ((1.0 - ratio) * glm::length(cartesian)) /
            (0.5 * glm::length(gradient));
        double correction = 0.0;

        double func;
        double denominator;
        double xMultiplier;
        double yMultiplier;
        double zMultiplier;
        double xMultiplier2;
        double yMultiplier2;
        double zMultiplier2;
        double xMultiplier3;
        double yMultiplier3;
        double zMultiplier3;

        do {
            lambda -= correction;

            xMultiplier = 1.0 / (1.0 + lambda * oneOverRadiiSquaredX);
            yMultiplier = 1.0 / (1.0 + lambda * oneOverRadiiSquaredY);
            zMultiplier = 1.0 / (1.0 + lambda * oneOverRadiiSquaredZ);

            xMultiplier2 = xMultiplier * xMultiplier;
            yMultiplier2 = yMultiplier * yMultiplier;
            zMultiplier2 = zMultiplier * zMultiplier;

            xMultiplier3 = xMultiplier2 * xMultiplier;
            yMultiplier3 = yMultiplier2 * yMultiplier;
            zMultiplier3 = zMultiplier2 * zMultiplier;

            func = x2 * xMultiplier2 + y2 * yMultiplier2 + z2 * zMultiplier2 - 1.0;

            // "denominator" here refers to the use of this expression in the velocity and acceleration
            // computations in the sections to follow.
            denominator =
            x2 * xMultiplier3 * oneOverRadiiSquaredX +
            y2 * yMultiplier3 * oneOverRadiiSquaredY +
            z2 * zMultiplier3 * oneOverRadiiSquaredZ;

            double derivative = -2.0 * denominator;

            correction = func / derivative;
        } while (std::abs(func) > Math::EPSILON12);

        return glm::dvec3(
            positionX * xMultiplier,
            positionY * yMultiplier,
            positionZ * zMultiplier
        );
    }
}
