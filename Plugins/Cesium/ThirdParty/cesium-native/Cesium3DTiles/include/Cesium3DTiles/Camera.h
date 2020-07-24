#pragma once

#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat3x3.hpp>
#include "BoundingVolume.h"
#include "CesiumGeometry/Plane.h"

namespace Cesium3DTiles {

    class CESIUM3DTILES_API Camera {
    public:
        // TODO: Add support for orthographic and off-center perspective frustums

        Camera(
            const glm::dvec3& position,
            const glm::dvec3& direction,
            const glm::dvec3& up,
            const glm::dvec2& viewportSize,
            double horizontalFieldOfView,
            double verticalFieldOfView
        );

        /// <summary>
        /// Gets the position of the camera in Earth-centered, Earth-fixed coordinates.
        /// </summary>
        const glm::dvec3& getPosition() const { return this->_position; }

        /// <summary>
        /// Gets the look direction of the camera in Earth-centered, Earth-fixed coordinates.
        /// </summary>
        const glm::dvec3& getDirection() const { return this->_direction; }

        /// <summary>
        /// Gets the up direction of the camera in Earth-centered, Earth-fixed coordinates.
        /// </summary>
        const glm::dvec3& getUp() const { return this->_up; }

        /// <summary>
        /// Gets the size of the viewport in pixels.
        /// </summary>
        const glm::dvec2& getViewportSize() const { return this->_viewportSize; }

        /// <summary>
        /// Gets the horizontal field-of-view angle in radians.
        /// </summary>
        double getHorizontalFieldOfView() const { return this->_horizontalFieldOfView; }

        /// <summary>
        /// Gets the vertical field-of-view angle in radians.
        /// </summary>
        double getVerticalFieldOfView() const { return this->_verticalFieldOfView; }

        /// <summary>
        /// Gets the denominator used in screen-space error (SSE) computations,
        /// <c>2.0 * tan(0.5 * verticalFieldOfView)</c>.
        /// </summary>
        double getScreenSpaceErrorDenominator() const { return this->_sseDenominator; }

        /// <summary>
        /// Updates the position and orientation of the camera.
        /// </summary>
        /// <param name="position">The new position.</param>
        /// <param name="direction">The new look direction vector.</param>
        /// <param name="up">The new up vector.</param>
        void updatePositionAndOrientation(const glm::dvec3& position, const glm::dvec3& direction, const glm::dvec3& up);

        /// <summary>
        /// Updates the camera's view parameters.
        /// </summary>
        /// <param name="viewportSize">The new size of the viewport.</param>
        /// <param name="horizontalFieldOfView">The horizontal field of view angle in radians.</param>
        /// <param name="verticalFieldOfView">The vertical field of view angle in radians.</param>
        void updateViewParameters(const glm::dvec2& viewportSize, double horizontalFieldOfView, double verticalFieldOfView);

        bool isBoundingVolumeVisible(const BoundingVolume& boundingVolume) const;

        double computeDistanceSquaredToBoundingVolume(const BoundingVolume& boundingVolume) const;
        double computeScreenSpaceError(double geometricError, double distance) const;

    private:
        void _updateCullingVolume();

        glm::dvec3 _position;
        glm::dvec3 _direction;
        glm::dvec3 _up;
        glm::dvec2 _viewportSize;
        double _horizontalFieldOfView;
        double _verticalFieldOfView;
        double _sseDenominator;

        Plane _leftPlane;
        Plane _rightPlane;
        Plane _topPlane;
        Plane _bottomPlane;
    };

}
