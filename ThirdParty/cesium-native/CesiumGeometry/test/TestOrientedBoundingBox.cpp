#define _USE_MATH_DEFINES
#include <cmath>
#include <optional>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include "catch2/catch.hpp"
#include "CesiumGeometry/OrientedBoundingBox.h"
#include "Cesium3DTiles/Camera.h"

using namespace CesiumGeometry;
using namespace Cesium3DTiles;

TEST_CASE("OrientedBoundingBox::intersectPlane") {
    struct TestCase {
        glm::dvec3 center;
        glm::dmat3 axes;
    };

    auto testCase = GENERATE(
        // untransformed
        TestCase { glm::dvec3(0.0), glm::dmat3(1.0) },
        // off-center
        TestCase { glm::dvec3(1.0, 0.0, 0.0), glm::dmat3(1.0) },
        TestCase { glm::dvec3(0.7, -1.8, 12.0), glm::dmat3(1.0) },
        // rotated
        TestCase { glm::dvec3(0.0), glm::rotate(glm::dmat4(), 1.2, glm::dvec3(0.5, 1.5, -1.2)) },
        // scaled
        TestCase { glm::dvec3(0.0), glm::scale(glm::dmat4(), glm::dvec3(1.5, 0.4, 20.6)) },
        TestCase { glm::dvec3(0.0), glm::scale(glm::dmat4(), glm::dvec3(0.0, 0.4, 20.6)) },
        TestCase { glm::dvec3(0.0), glm::scale(glm::dmat4(), glm::dvec3(1.5, 0.0, 20.6)) },
        TestCase { glm::dvec3(0.0), glm::scale(glm::dmat4(), glm::dvec3(1.5, 0.4, 0.0)) },
        TestCase { glm::dvec3(0.0), glm::scale(glm::dmat4(), glm::dvec3(0.0, 0.0, 0.0)) },
        // arbitrary box
        TestCase {
            glm::dvec3(-5.1, 0.0, 0.1),
            glm::rotate(
                glm::scale(glm::dmat4(), glm::dvec3(1.5, 80.4, 2.6)),
                1.2,
                glm::dvec3(0.5, 1.5, -1.2)
            )
        }
    );

    SECTION("test corners, edges, faces") {
        const double SQRT1_2 = pow(1.0 / 2.0, 1 / 2.0);
        const double SQRT3_4 = pow(3.0 / 4.0, 1 / 2.0);

        auto box = OrientedBoundingBox(testCase.center, testCase.axes * 0.5);
        
        std::string s = glm::to_string(box.getHalfAxes());

        auto planeNormXform = [&testCase](double nx, double ny, double nz, double dist) {
            auto n = glm::dvec3(nx, ny, nz);
            auto arb = glm::dvec3(357, 924, 258);
            auto p0 = glm::normalize(n) * -dist;
            auto tang = glm::normalize(glm::cross(n, arb));
            auto binorm = glm::normalize(glm::cross(n, tang));

            p0 = testCase.axes * p0;
            tang = testCase.axes * tang;
            binorm = testCase.axes * binorm;

            n = glm::cross(tang, binorm);
            if (glm::length(n) == 0.0) {
                return std::optional<Plane>();
            }
            n = glm::normalize(n);

            p0 += testCase.center;
            double d = -glm::dot(p0, n);
            if (std::abs(d) > 0.0001 && glm::dot(n, n) > 0.0001) {
                return std::optional(Plane(n, d));
            }

            return std::optional<Plane>();
        };

        std::optional<Plane> pl;

        // Tests against faces

        pl = planeNormXform(+1.0, +0.0, +0.0, 0.50001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
        pl = planeNormXform(-1.0, +0.0, +0.0, 0.50001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
        pl = planeNormXform(+0.0, +1.0, +0.0, 0.50001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
        pl = planeNormXform(+0.0, -1.0, +0.0, 0.50001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
        pl = planeNormXform(+0.0, +0.0, +1.0, 0.50001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
        pl = planeNormXform(+0.0, +0.0, -1.0, 0.50001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));

        pl = planeNormXform(+1.0, +0.0, +0.0, 0.49999);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(-1.0, +0.0, +0.0, 0.49999);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(+0.0, +1.0, +0.0, 0.49999);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(+0.0, -1.0, +0.0, 0.49999);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(+0.0, +0.0, +1.0, 0.49999);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(+0.0, +0.0, -1.0, 0.49999);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));

        pl = planeNormXform(+1.0, +0.0, +0.0, -0.49999);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(-1.0, +0.0, +0.0, -0.49999);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(+0.0, +1.0, +0.0, -0.49999);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(+0.0, -1.0, +0.0, -0.49999);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(+0.0, +0.0, +1.0, -0.49999);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(+0.0, +0.0, -1.0, -0.49999);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));

        pl = planeNormXform(+1.0, +0.0, +0.0, -0.50001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
        pl = planeNormXform(-1.0, +0.0, +0.0, -0.50001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
        pl = planeNormXform(+0.0, +1.0, +0.0, -0.50001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
        pl = planeNormXform(+0.0, -1.0, +0.0, -0.50001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
        pl = planeNormXform(+0.0, +0.0, +1.0, -0.50001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
        pl = planeNormXform(+0.0, +0.0, -1.0, -0.50001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));

        // Tests against edges

        pl = planeNormXform(+1.0, +1.0, +0.0, SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
        pl = planeNormXform(+1.0, -1.0, +0.0, SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
        pl = planeNormXform(-1.0, +1.0, +0.0, SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
        pl = planeNormXform(-1.0, -1.0, +0.0, SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
        pl = planeNormXform(+1.0, +0.0, +1.0, SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
        pl = planeNormXform(+1.0, +0.0, -1.0, SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
        pl = planeNormXform(-1.0, +0.0, +1.0, SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
        pl = planeNormXform(-1.0, +0.0, -1.0, SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
        pl = planeNormXform(+0.0, +1.0, +1.0, SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
        pl = planeNormXform(+0.0, +1.0, -1.0, SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
        pl = planeNormXform(+0.0, -1.0, +1.0, SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
        pl = planeNormXform(+0.0, -1.0, -1.0, SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));

        pl = planeNormXform(+1.0, +1.0, +0.0, SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(+1.0, -1.0, +0.0, SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(-1.0, +1.0, +0.0, SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(-1.0, -1.0, +0.0, SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(+1.0, +0.0, +1.0, SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(+1.0, +0.0, -1.0, SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(-1.0, +0.0, +1.0, SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(-1.0, +0.0, -1.0, SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(+0.0, +1.0, +1.0, SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(+0.0, +1.0, -1.0, SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(+0.0, -1.0, +1.0, SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(+0.0, -1.0, -1.0, SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));

        pl = planeNormXform(+1.0, +1.0, +0.0, -SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(+1.0, -1.0, +0.0, -SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(-1.0, +1.0, +0.0, -SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(-1.0, -1.0, +0.0, -SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(+1.0, +0.0, +1.0, -SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(+1.0, +0.0, -1.0, -SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(-1.0, +0.0, +1.0, -SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(-1.0, +0.0, -1.0, -SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(+0.0, +1.0, +1.0, -SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(+0.0, +1.0, -1.0, -SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(+0.0, -1.0, +1.0, -SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(+0.0, -1.0, -1.0, -SQRT1_2 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));

        pl = planeNormXform(+1.0, +1.0, +0.0, -SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
        pl = planeNormXform(+1.0, -1.0, +0.0, -SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
        pl = planeNormXform(-1.0, +1.0, +0.0, -SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
        pl = planeNormXform(-1.0, -1.0, +0.0, -SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
        pl = planeNormXform(+1.0, +0.0, +1.0, -SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
        pl = planeNormXform(+1.0, +0.0, -1.0, -SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
        pl = planeNormXform(-1.0, +0.0, +1.0, -SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
        pl = planeNormXform(-1.0, +0.0, -1.0, -SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
        pl = planeNormXform(+0.0, +1.0, +1.0, -SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
        pl = planeNormXform(+0.0, +1.0, -1.0, -SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
        pl = planeNormXform(+0.0, -1.0, +1.0, -SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
        pl = planeNormXform(+0.0, -1.0, -1.0, -SQRT1_2 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));

        // Tests against corners

        pl = planeNormXform(+1.0, +1.0, +1.0, SQRT3_4 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
        pl = planeNormXform(+1.0, +1.0, -1.0, SQRT3_4 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
        pl = planeNormXform(+1.0, -1.0, +1.0, SQRT3_4 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
        pl = planeNormXform(+1.0, -1.0, -1.0, SQRT3_4 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
        pl = planeNormXform(-1.0, +1.0, +1.0, SQRT3_4 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
        pl = planeNormXform(-1.0, +1.0, -1.0, SQRT3_4 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
        pl = planeNormXform(-1.0, -1.0, +1.0, SQRT3_4 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));
        pl = planeNormXform(-1.0, -1.0, -1.0, SQRT3_4 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Inside));

        pl = planeNormXform(+1.0, +1.0, +1.0, SQRT3_4 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(+1.0, +1.0, -1.0, SQRT3_4 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(+1.0, -1.0, +1.0, SQRT3_4 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(+1.0, -1.0, -1.0, SQRT3_4 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(-1.0, +1.0, +1.0, SQRT3_4 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(-1.0, +1.0, -1.0, SQRT3_4 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(-1.0, -1.0, +1.0, SQRT3_4 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(-1.0, -1.0, -1.0, SQRT3_4 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));

        pl = planeNormXform(+1.0, +1.0, +1.0, -SQRT3_4 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(+1.0, +1.0, -1.0, -SQRT3_4 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(+1.0, -1.0, +1.0, -SQRT3_4 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(+1.0, -1.0, -1.0, -SQRT3_4 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(-1.0, +1.0, +1.0, -SQRT3_4 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(-1.0, +1.0, -1.0, -SQRT3_4 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(-1.0, -1.0, +1.0, -SQRT3_4 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));
        pl = planeNormXform(-1.0, -1.0, -1.0, -SQRT3_4 + 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Intersecting));

        pl = planeNormXform(+1.0, +1.0, +1.0, -SQRT3_4 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
        pl = planeNormXform(+1.0, +1.0, -1.0, -SQRT3_4 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
        pl = planeNormXform(+1.0, -1.0, +1.0, -SQRT3_4 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
        pl = planeNormXform(+1.0, -1.0, -1.0, -SQRT3_4 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
        pl = planeNormXform(-1.0, +1.0, +1.0, -SQRT3_4 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
        pl = planeNormXform(-1.0, +1.0, -1.0, -SQRT3_4 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
        pl = planeNormXform(-1.0, -1.0, +1.0, -SQRT3_4 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
        pl = planeNormXform(-1.0, -1.0, -1.0, -SQRT3_4 - 0.00001);
        CHECK((!pl || box.intersectPlane(*pl) == CullingResult::Outside));
    }
}

TEST_CASE("OrientedBoundingBox examples") {
    {
    //! [Constructor]
    // Create an OrientedBoundingBox using a transformation matrix, a position where the box will be translated, and a scale.
    glm::dvec3 center = glm::dvec3(1.0, 0.0, 0.0);
    glm::dmat3 halfAxes = glm::dmat3(
        glm::dvec3(1.0, 0.0, 0.0),
        glm::dvec3(0.0, 3.0, 0.0),
        glm::dvec3(0.0, 0.0, 2.0)
    );

    auto obb = OrientedBoundingBox(center, halfAxes);
    //! [Constructor]
    (void)obb;
    }

    {
    auto anyOldBoxArray = []() {
        return std::vector<OrientedBoundingBox> {
            { glm::dvec3(1.0, 0.0, 0.0), glm::dmat3(1.0) },
            { glm::dvec3(2.0, 0.0, 0.0), glm::dmat3(1.0) }
        };
    };
    auto anyOldCameraPosition = []() {
        return glm::dvec3(0.0, 0.0, 0.0);
    };

    //! [distanceSquaredTo]
    // Sort bounding boxes from back to front
    glm::dvec3 cameraPosition = anyOldCameraPosition();
    std::vector<OrientedBoundingBox> boxes = anyOldBoxArray();
    std::sort(boxes.begin(), boxes.end(), [&cameraPosition](auto& a, auto& b) {
        return a.computeDistanceSquaredToPosition(cameraPosition) > b.computeDistanceSquaredToPosition(cameraPosition);
    });
    //! [distanceSquaredTo]

    CHECK(boxes[0].getCenter().x == 2.0);
    CHECK(boxes[1].getCenter().x == 1.0);
    }
}