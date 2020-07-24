#include "CesiumGeometry/OrientedBoundingBox.h"
#include <glm/geometric.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <stdexcept>
#include "CesiumGeometry/Plane.h"
#include "Cesium3DTiles/Ellipsoid.h"
#include "CesiumUtility/Math.h"
#include "Cesium3DTiles/EllipsoidTangentPlane.h"

namespace Cesium3DTiles {
    static OrientedBoundingBox fromPlaneExtents(
        const glm::dvec3& planeOrigin,
        const glm::dvec3& planeXAxis,
        const glm::dvec3& planeYAxis,
        const glm::dvec3& planeZAxis,
        double minimumX,
        double maximumX,
        double minimumY,
        double maximumY,
        double minimumZ,
        double maximumZ
    ) {
        glm::dmat3 halfAxes(planeXAxis, planeYAxis, planeZAxis);

        glm::dvec3 centerOffset(
            (minimumX + maximumX) / 2.0,
            (minimumY + maximumY) / 2.0,
            (minimumZ + maximumZ) / 2.0
        );

        glm::dvec3 scale(
            (maximumX - minimumX) / 2.0,
            (maximumY - minimumY) / 2.0,
            (maximumZ - minimumZ) / 2.0
        );

        glm::dmat3 scaledHalfAxes(
            halfAxes[0] * scale.x,
            halfAxes[1] * scale.y,
            halfAxes[2] * scale.z
        );

        return OrientedBoundingBox(
            planeOrigin + (halfAxes * centerOffset),
            scaledHalfAxes
        );
    }

    /*static*/ OrientedBoundingBox OrientedBoundingBox::fromRectangle(
        const Rectangle& rectangle,
        double minimumHeight,
        double maximumHeight,
        const Ellipsoid& ellipsoid
    ) {
        //>>includeStart('debug', pragmas.debug);
        if (!Math::equalsEpsilon(ellipsoid.getRadii().x, ellipsoid.getRadii().y, Math::EPSILON15)) {
            throw std::runtime_error("Ellipsoid must be an ellipsoid of revolution (radii.x == radii.y)");
        }
        //>>includeEnd('debug');

        double minX, maxX, minY, maxY, minZ, maxZ;
        Plane plane(glm::dvec3(0.0, 0.0, 1.0), 0.0);

        if (rectangle.computeWidth() <= Math::PI) {
            // The bounding box will be aligned with the tangent plane at the center of the rectangle.
            Cartographic tangentPointCartographic = rectangle.computeCenter();
            glm::dvec3 tangentPoint = ellipsoid.cartographicToCartesian(tangentPointCartographic);
            EllipsoidTangentPlane tangentPlane(tangentPoint, ellipsoid);
            plane = tangentPlane.getPlane();

            // If the rectangle spans the equator, CW is instead aligned with the equator (because it sticks out the farthest at the equator).
            double lonCenter = tangentPointCartographic.longitude;
            double latCenter =
                rectangle.getSouth() < 0.0 && rectangle.getNorth() > 0.0
                    ? 0.0
                    : tangentPointCartographic.latitude;

            // Compute XY extents using the rectangle at maximum height
            Cartographic perimeterCartographicNC(lonCenter, rectangle.getNorth(), maximumHeight);
            Cartographic perimeterCartographicNW(rectangle.getWest(), rectangle.getNorth(), maximumHeight);
            Cartographic perimeterCartographicCW(rectangle.getWest(), latCenter, maximumHeight);
            Cartographic perimeterCartographicSW(rectangle.getWest(), rectangle.getSouth(), maximumHeight);
            Cartographic perimeterCartographicSC(lonCenter, rectangle.getSouth(), maximumHeight);

            glm::dvec3 perimeterCartesianNC = ellipsoid.cartographicToCartesian(perimeterCartographicNC);
            glm::dvec3 perimeterCartesianNW = ellipsoid.cartographicToCartesian(perimeterCartographicNW);
            glm::dvec3 perimeterCartesianCW = ellipsoid.cartographicToCartesian(perimeterCartographicCW);
            glm::dvec3 perimeterCartesianSW = ellipsoid.cartographicToCartesian(perimeterCartographicSW);
            glm::dvec3 perimeterCartesianSC = ellipsoid.cartographicToCartesian(perimeterCartographicSC);

            glm::dvec2 perimeterProjectedNC = tangentPlane.projectPointToNearestOnPlane(perimeterCartesianNC);
            glm::dvec2 perimeterProjectedNW = tangentPlane.projectPointToNearestOnPlane(perimeterCartesianNW);
            glm::dvec2 perimeterProjectedCW = tangentPlane.projectPointToNearestOnPlane(perimeterCartesianCW);
            glm::dvec2 perimeterProjectedSW = tangentPlane.projectPointToNearestOnPlane(perimeterCartesianSW);
            glm::dvec2 perimeterProjectedSC = tangentPlane.projectPointToNearestOnPlane(perimeterCartesianSC);

            minX = std::min(
                std::min(
                    perimeterProjectedNW.x,
                    perimeterProjectedCW.x
                ),
                perimeterProjectedSW.x
            );

            maxX = -minX; // symmetrical

            maxY = std::max(perimeterProjectedNW.y, perimeterProjectedNC.y);
            minY = std::min(perimeterProjectedSW.y, perimeterProjectedSC.y);

            // Compute minimum Z using the rectangle at minimum height, since it will be deeper than the maximum height
            perimeterCartographicNW.height = perimeterCartographicSW.height = minimumHeight;
            perimeterCartesianNW = ellipsoid.cartographicToCartesian(perimeterCartographicNW);
            perimeterCartesianSW = ellipsoid.cartographicToCartesian(perimeterCartographicSW);

            minZ = std::min(
                plane.getPointDistance(perimeterCartesianNW),
                plane.getPointDistance(perimeterCartesianSW)
            );
            maxZ = maximumHeight; // Since the tangent plane touches the surface at height = 0, this is okay

            return fromPlaneExtents(
                tangentPlane.getOrigin(),
                tangentPlane.getXAxis(),
                tangentPlane.getYAxis(),
                tangentPlane.getZAxis(),
                minX,
                maxX,
                minY,
                maxY,
                minZ,
                maxZ
            );
        }

        // Handle the case where rectangle width is greater than PI (wraps around more than half the ellipsoid).
        bool fullyAboveEquator = rectangle.getSouth() > 0.0;
        bool fullyBelowEquator = rectangle.getNorth() < 0.0;
        double latitudeNearestToEquator = fullyAboveEquator
            ? rectangle.getSouth()
            : fullyBelowEquator
            ? rectangle.getNorth()
            : 0.0;
        double centerLongitude = rectangle.computeCenter().longitude;

        // Plane is located at the rectangle's center longitude and the rectangle's latitude that is closest to the equator. It rotates around the Z axis.
        // This results in a better fit than the obb approach for smaller rectangles, which orients with the rectangle's center normal.
        glm::dvec3 planeOrigin = ellipsoid.cartographicToCartesian(
            Cartographic(centerLongitude, latitudeNearestToEquator, maximumHeight)
        );
        planeOrigin.z = 0.0; // center the plane on the equator to simpify plane normal calculation
        bool isPole =
            std::abs(planeOrigin.x) < Math::EPSILON10 &&
            std::abs(planeOrigin.y) < Math::EPSILON10;
        glm::dvec3 planeNormal = !isPole
            ? glm::normalize(planeOrigin)
            : glm::dvec3(1.0, 0.0, 0.0);
        glm::dvec3 planeYAxis(0.0, 0.0, 1.0);
        glm::dvec3 planeXAxis = glm::cross(planeNormal, planeYAxis);
        plane = Plane(planeOrigin, planeNormal);

        // Get the horizon point relative to the center. This will be the farthest extent in the plane's X dimension.
        glm::dvec3 horizonCartesian = ellipsoid.cartographicToCartesian(
            Cartographic(centerLongitude + Math::PI_OVER_TWO, latitudeNearestToEquator, maximumHeight)
        );
        maxX = glm::dot(
            plane.projectPointOntoPlane(horizonCartesian),
            planeXAxis
        );
        minX = -maxX; // symmetrical

        // Get the min and max Y, using the height that will give the largest extent
        maxY = ellipsoid.cartographicToCartesian(Cartographic(0.0, rectangle.getNorth(), fullyBelowEquator ? minimumHeight : maximumHeight)).z;
        minY = ellipsoid.cartographicToCartesian(Cartographic(0.0, rectangle.getSouth(), fullyAboveEquator ? minimumHeight : maximumHeight)).z;
        glm::dvec3 farZ = ellipsoid.cartographicToCartesian(Cartographic(rectangle.getEast(), latitudeNearestToEquator, maximumHeight));
        minZ = plane.getPointDistance(farZ);
        maxZ = 0.0; // plane origin starts at maxZ already

        // min and max are local to the plane axes
        return fromPlaneExtents(
            planeOrigin,
            planeXAxis,
            planeYAxis,
            planeNormal,
            minX,
            maxX,
            minY,
            maxY,
            minZ,
            maxZ
        );
    }

    CullingResult OrientedBoundingBox::intersectPlane(const Plane& plane) const {
        glm::dvec3 normal = plane.getNormal();

        const glm::dmat3& halfAxes = this->getHalfAxes();
        const glm::dvec3& xAxisDirectionAndHalfLength = halfAxes[0];
        const glm::dvec3& yAxisDirectionAndHalfLength = halfAxes[1];
        const glm::dvec3& zAxisDirectionAndHalfLength = halfAxes[2];

        // plane is used as if it is its normal; the first three components are assumed to be normalized
        double radEffective =
            std::abs(
                normal.x * xAxisDirectionAndHalfLength.x +
                normal.y * xAxisDirectionAndHalfLength.y +
                normal.z * xAxisDirectionAndHalfLength.z
            ) +
            std::abs(
                normal.x * yAxisDirectionAndHalfLength.x +
                normal.y * yAxisDirectionAndHalfLength.y +
                normal.z * yAxisDirectionAndHalfLength.z
            ) +
            std::abs(
                normal.x * zAxisDirectionAndHalfLength.x +
                normal.y * zAxisDirectionAndHalfLength.y +
                normal.z * zAxisDirectionAndHalfLength.z
            );

        double distanceToPlane = ::glm::dot(normal, this->getCenter()) + plane.getDistance();

        if (distanceToPlane <= -radEffective) {
            // The entire box is on the negative side of the plane normal
            return CullingResult::Outside;
        } else if (distanceToPlane >= radEffective) {
            // The entire box is on the positive side of the plane normal
            return CullingResult::Inside;
        }
        return CullingResult::Intersecting;
    }

    double OrientedBoundingBox::computeDistanceSquaredToPosition(const glm::dvec3& position) const {
        glm::dvec3 offset = position - this->getCenter();

        const glm::dmat3& halfAxes = this->getHalfAxes();
        glm::dvec3 u = halfAxes[0];
        glm::dvec3 v = halfAxes[1];
        glm::dvec3 w = halfAxes[2];

        double uHalf = glm::length(u);
        double vHalf = glm::length(v);
        double wHalf = glm::length(w);

        u /= uHalf;
        v /= vHalf;
        w /= wHalf;

        glm::dvec3 pPrime(
            glm::dot(offset, u),
            glm::dot(offset, v),
            glm::dot(offset, w)
        );

        double distanceSquared = 0.0;
        double d;

        if (pPrime.x < -uHalf) {
            d = pPrime.x + uHalf;
            distanceSquared += d * d;
        }
        else if (pPrime.x > uHalf) {
            d = pPrime.x - uHalf;
            distanceSquared += d * d;
        }

        if (pPrime.y < -vHalf) {
            d = pPrime.y + vHalf;
            distanceSquared += d * d;
        }
        else if (pPrime.y > vHalf) {
            d = pPrime.y - vHalf;
            distanceSquared += d * d;
        }

        if (pPrime.z < -wHalf) {
            d = pPrime.z + wHalf;
            distanceSquared += d * d;
        }
        else if (pPrime.z > wHalf) {
            d = pPrime.z - wHalf;
            distanceSquared += d * d;
        }

        return distanceSquared;
    }

}
