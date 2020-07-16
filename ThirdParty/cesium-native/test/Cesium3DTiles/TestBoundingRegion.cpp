#include "catch2/catch.hpp"
#include "Cesium3DTiles/BoundingRegion.h"
#include "Cesium3DTiles/Rectangle.h"
#include "Cesium3DTiles/Ellipsoid.h"
#include "Cesium3DTiles/Math.h"
#include "Cesium3DTiles/Plane.h"

TEST_CASE("BoundingRegion") {
    using namespace Cesium3DTiles;

    SECTION("computeDistanceSquaredToPosition") {
        struct TestCase {
            double longitude;
            double latitude;
            double height;
            double expectedDistance;
        };

        const double offset = 0.0001;

        auto updateDistance = [](TestCase& testCase, double longitude, double latitude, double height) -> TestCase {
            glm::dvec3 boxPosition = Ellipsoid::WGS84.cartographicToCartesian(Cartographic(longitude, latitude, height));
            glm::dvec3 position = Ellipsoid::WGS84.cartographicToCartesian(Cartographic(testCase.longitude, testCase.latitude, testCase.height));
            testCase.expectedDistance = glm::distance(boxPosition, position);
            return testCase;
        };

        BoundingRegion region(
            Rectangle(-0.001, -0.001, 0.001, 0.001),
            0.0,
            10.0
        );

        auto testCase = GENERATE_COPY(
            // Inside bounding region
            TestCase { region.getWest() + Math::EPSILON6, region.getSouth(), region.getMinimumHeight(), 0.0 },
            // Outside bounding region
            TestCase { region.getWest(), region.getSouth(), region.getMaximumHeight() + 1.0, 1.0 },
            // Inside rectangle, above height
            TestCase { 0.0, 0.0, 20.0, 10.0 },
            // Inside rectangle, below height
            TestCase { 0.0, 0.0, 5.0, 0.0 },
            // From southwest
            updateDistance(TestCase {
                region.getEast() + offset,
                region.getNorth() + offset,
                0.0,
                0.0
            }, region.getEast(), region.getNorth(), 0.0),
            // From northeast
            updateDistance(TestCase {
                region.getWest() - offset,
                region.getSouth() - offset,
                0.0,
                0.0
            }, region.getWest(), region.getSouth(), 0.0)
        );

        glm::dvec3 position = Ellipsoid::WGS84.cartographicToCartesian(
            Cartographic(testCase.longitude, testCase.latitude, testCase.height)
        );
        CHECK(Math::equalsEpsilon(sqrt(region.computeDistanceSquaredToPosition(position)), testCase.expectedDistance, Math::EPSILON6));
    }

    SECTION("intersectPlane") {
        BoundingRegion region(
            Rectangle(0.0, 0.0, 1.0, 1.0),
            0.0,
            1.0
        );

        Plane plane(
            glm::normalize(Ellipsoid::WGS84.cartographicToCartesian(Cartographic(0.0, 0.0, 1.0))),
            -glm::distance(glm::dvec3(0.0), Ellipsoid::WGS84.cartographicToCartesian(Cartographic(0.0, 0.0, 0.0)))
        );

        CHECK(region.intersectPlane(plane) == CullingResult::Intersecting);
    }
}
