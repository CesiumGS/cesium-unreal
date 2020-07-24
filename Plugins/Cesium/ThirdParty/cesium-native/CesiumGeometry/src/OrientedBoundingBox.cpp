#include "CesiumGeometry/OrientedBoundingBox.h"
#include <glm/geometric.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <stdexcept>
#include "CesiumGeometry/Plane.h"
#include "CesiumUtility/Math.h"

namespace Cesium3DTiles {
    /*static*/ OrientedBoundingBox OrientedBoundingBox::fromPlaneExtents(
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
