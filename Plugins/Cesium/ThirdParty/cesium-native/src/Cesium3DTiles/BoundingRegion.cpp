#include "Cesium3DTiles/BoundingRegion.h"
#include "Cesium3DTiles/IntersectionTests.h"
#include "Cesium3DTiles/Ray.h"
#include "Cesium3DTiles/Plane.h"

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
        BoundingRegion(Rectangle(west, south, east, north), minimumHeight, maximumHeight)
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
        _boundingBox(BoundingBox::fromRectangle(rectangle, minimumHeight, maximumHeight)),
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

}
