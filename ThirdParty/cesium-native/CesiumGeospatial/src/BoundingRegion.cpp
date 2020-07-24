#include "CesiumGeospatial/BoundingRegion.h"
#include "CesiumGeometry/IntersectionTests.h"
#include "CesiumGeometry/Ray.h"
#include "CesiumGeometry/Plane.h"
#include "CesiumUtility/Math.h"
#include "CesiumGeospatial/EllipsoidTangentPlane.h"
#include <stdexcept>

namespace Cesium3DTiles {

    BoundingRegion::BoundingRegion(
        double west,
        double south,
        double east,
        double north,
        double minimumHeight,
        double maximumHeight,
        const Ellipsoid& ellipsoid
    ) :
        BoundingRegion(Rectangle(west, south, east, north), minimumHeight, maximumHeight, ellipsoid)
    {
    }

    BoundingRegion::BoundingRegion(
        const Rectangle& rectangle,
        double minimumHeight,
        double maximumHeight,
        const Ellipsoid& ellipsoid
    ) :
        _rectangle(rectangle),
        _minimumHeight(minimumHeight),
        _maximumHeight(maximumHeight),
        _boundingBox(BoundingRegion::_computeBoundingBox(rectangle, minimumHeight, maximumHeight, ellipsoid)),
        _southwestCornerCartesian(ellipsoid.cartographicToCartesian(rectangle.getSouthwest())),
        _northeastCornerCartesian(ellipsoid.cartographicToCartesian(rectangle.getNortheast())),
        _westNormal(),
        _eastNormal(),
        _southNormal(),
        _northNormal()
    {
        // The middle latitude on the western edge.
        glm::dvec3 westernMidpointCartesian = ellipsoid.cartographicToCartesian(
            Cartographic(rectangle.getWest(), (rectangle.getSouth() + rectangle.getNorth()) * 0.5, 0.0)
        );

        // Compute the normal of the plane on the western edge of the tile.
        this->_westNormal = glm::normalize(glm::cross(
            westernMidpointCartesian,
            glm::dvec3(0.0, 0.0, 1.0)
        ));

        // The middle latitude on the eastern edge.
        glm::dvec3 easternMidpointCartesian = ellipsoid.cartographicToCartesian(
            Cartographic(rectangle.getEast(), (rectangle.getSouth() + rectangle.getNorth()) * 0.5, 0.0)
        );

        // Compute the normal of the plane on the eastern edge of the tile.
        this->_eastNormal = glm::normalize(glm::cross(
            glm::dvec3(0.0, 0.0, 1.0),
            easternMidpointCartesian
        ));

        // Compute the normal of the plane bounding the southern edge of the tile.
        glm::dvec3 westVector = westernMidpointCartesian - easternMidpointCartesian;
        glm::dvec3 eastWestNormal = glm::normalize(westVector);

        double south = rectangle.getSouth();
        glm::dvec3 southSurfaceNormal;

        if (south > 0.0) {
            // Compute a plane that doesn't cut through the tile.
            glm::dvec3 southCenterCartesian = ellipsoid.cartographicToCartesian(
                Cartographic((rectangle.getWest() + rectangle.getEast()) * 0.5, south, 0.0)
            );
            Plane westPlane(
                this->_southwestCornerCartesian,
                this->_westNormal
            );

            // Find a point that is on the west and the south planes
            this->_southwestCornerCartesian = IntersectionTests::rayPlane(
                Ray(southCenterCartesian, eastWestNormal),
                westPlane
            ).value();
            southSurfaceNormal = ellipsoid.geodeticSurfaceNormal(southCenterCartesian);
        } else {
            southSurfaceNormal = ellipsoid.geodeticSurfaceNormal(rectangle.getSoutheast());
        }
        this->_southNormal = glm::normalize(glm::cross(
            southSurfaceNormal,
            westVector
        ));

        // Compute the normal of the plane bounding the northern edge of the tile.
        double north = rectangle.getNorth();
        glm::dvec3 northSurfaceNormal;
        if (north < 0.0) {
            // Compute a plane that doesn't cut through the tile.
            glm::dvec3 northCenterCartesian = ellipsoid.cartographicToCartesian(
                Cartographic((rectangle.getWest() + rectangle.getEast()) * 0.5, north, 0.0)
            );

            Plane eastPlane(
                this->_northeastCornerCartesian,
                this->_eastNormal
            );

            // Find a point that is on the east and the north planes
            this->_northeastCornerCartesian = IntersectionTests::rayPlane(
                Ray(northCenterCartesian, -eastWestNormal),
                eastPlane
            ).value();
            northSurfaceNormal = ellipsoid.geodeticSurfaceNormal(northCenterCartesian);
        } else {
            northSurfaceNormal = ellipsoid.geodeticSurfaceNormal(rectangle.getNorthwest());
        }
        this->_northNormal = glm::normalize(glm::cross(
            westVector,
            northSurfaceNormal
        ));
    }

    CullingResult BoundingRegion::intersectPlane(const Plane& plane) const {
        return this->_boundingBox.intersectPlane(plane);
    }

    double BoundingRegion::computeDistanceSquaredToPosition(const glm::dvec3& position, const Ellipsoid& ellipsoid) const {
        std::optional<Cartographic> cartographic = ellipsoid.cartesianToCartographic(position);
        if (!cartographic) {
            return 0.0;
        }

        return this->computeDistanceSquaredToPosition(cartographic.value(), position);
    }

    double BoundingRegion::computeDistanceSquaredToPosition(const Cartographic& position, const Ellipsoid& ellipsoid) const {
        return this->computeDistanceSquaredToPosition(position, ellipsoid.cartographicToCartesian(position));
    }

    double BoundingRegion::computeDistanceSquaredToPosition(const Cartographic& cartographicPosition, const glm::dvec3& cartesianPosition) const {
        double result = 0.0;
        if (!this->_rectangle.contains(cartographicPosition)) {
            const glm::dvec3& southwestCornerCartesian = this->_southwestCornerCartesian;
            const glm::dvec3& northeastCornerCartesian = this->_northeastCornerCartesian;
            const glm::dvec3& westNormal = this->_westNormal;
            const glm::dvec3& southNormal = this->_southNormal;
            const glm::dvec3& eastNormal = this->_eastNormal;
            const glm::dvec3& northNormal = this->_northNormal;

            glm::dvec3 vectorFromSouthwestCorner = cartesianPosition - southwestCornerCartesian;
            double distanceToWestPlane = glm::dot(vectorFromSouthwestCorner, westNormal);
            double distanceToSouthPlane = glm::dot(vectorFromSouthwestCorner, southNormal);

            glm::dvec3 vectorFromNortheastCorner = cartesianPosition - northeastCornerCartesian;
            double distanceToEastPlane = glm::dot(vectorFromNortheastCorner, eastNormal);
            double distanceToNorthPlane = glm::dot(vectorFromNortheastCorner, northNormal);

            if (distanceToWestPlane > 0.0) {
                result += distanceToWestPlane * distanceToWestPlane;
            } else if (distanceToEastPlane > 0.0) {
                result += distanceToEastPlane * distanceToEastPlane;
            }

            if (distanceToSouthPlane > 0.0) {
                result += distanceToSouthPlane * distanceToSouthPlane;
            } else if (distanceToNorthPlane > 0.0) {
                result += distanceToNorthPlane * distanceToNorthPlane;
            }
        }

        double cameraHeight = cartographicPosition.height;
        double minimumHeight = this->_minimumHeight;
        double maximumHeight = this->_maximumHeight;

        if (cameraHeight > maximumHeight) {
            double distanceAboveTop = cameraHeight - maximumHeight;
            result += distanceAboveTop * distanceAboveTop;
        } else if (cameraHeight < minimumHeight) {
            double distanceBelowBottom = minimumHeight - cameraHeight;
            result += distanceBelowBottom * distanceBelowBottom;
        }

        return result;
    }

    /*static*/ OrientedBoundingBox BoundingRegion::_computeBoundingBox(
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

            return OrientedBoundingBox::fromPlaneExtents(
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
        return OrientedBoundingBox::fromPlaneExtents(
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

}
