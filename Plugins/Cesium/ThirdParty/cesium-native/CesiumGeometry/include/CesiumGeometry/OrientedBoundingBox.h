#pragma once

#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>
#include "CesiumGeometry/Library.h"
#include "CesiumGeometry/CullingResult.h"

namespace CesiumGeometry {

    class Plane;

    /**
     * @brief A {@link BoundingVolume} defined as a closed and convex cuboid with any orientation.
     * 
     * @see BoundingSphere
     * @see BoundingRegion
     */
    class CESIUMGEOMETRY_API OrientedBoundingBox {
    public:
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
        );

        /**
         * @brief Constructs a new instance.
         * 
         * @param center The center of the box.
         * @param halfAxes The three orthogonal half-axes of the bounding box. Equivalently, the
         *                 transformation matrix to rotate and scale a 0x0x0 cube centered at the
         *                 origin.
         * 
         * @snippet TestOrientedBoundingBox.cpp Constructor
         */
        OrientedBoundingBox(
            const glm::dvec3& center,
            const glm::dmat3& halfAxes
        ) :
            _center(center),
            _halfAxes(halfAxes)
        {
        }

        /**
         * @brief Gets the center of the box.
         */
        const glm::dvec3& getCenter() const { return this->_center; }

        /**
         * @brief Gets the transformation matrix, to rotate and scale the box to the right position and size.
         */
        const glm::dmat3& getHalfAxes() const { return this->_halfAxes; }

        /**
         * @brief Determines on which side of a plane the bounding box is located.
         * 
         * @param plane The plane to test against.
         * @return
         *  * {@link CullingResult::Inside} if the entire box is on the side of the plane the normal is pointing.
         *  * {@link CullingResult::Outside} if the entire box is on the opposite side.
         *  * {@link CullingResult::Intersecting} if the box intersects the plane.
         */
        CullingResult intersectPlane(const Plane& plane) const;

        /**
         * @brief Computes the distance squared from a given position to the closest point on this bounding volume.
         * The bounding volume and the position must be expressed in the same coordinate system.
         * 
         * @param position The position
         * @return The estimated distance squared from the bounding box to the point.
         * 
         * @snippet TestOrientedBoundingBox.cpp distanceSquaredTo
         */
        double computeDistanceSquaredToPosition(const glm::dvec3& position) const;

    private:
        glm::dvec3 _center;
        glm::dmat3 _halfAxes;
    };

}
