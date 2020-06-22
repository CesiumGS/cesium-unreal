#pragma once

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/mat3x3.hpp"

namespace Cesium3DTiles {

    class Camera {
    public:
        /// <summary>
        /// The position of the camera in Earth-centered, Earth-fixed coordinates.
        /// </summary>
        glm::dvec3 position;

        /// <summary>
        /// The look direction of the camera in Earth-centered, Earth-fixed coordinates.
        /// </summary>
        glm::dvec3 direction;

        /// <summary>
        /// The up direction of the camera in Earth-centered, Earth-fixed coordinates.
        /// </summary>
        glm::dvec3 up;

        /// <summary>
        /// The size of the viewport in pixels.
        /// </summary>
        glm::dvec2 viewportSize;

        /// <summary>
        /// The horizontal field-of-view angle in radians.
        /// </summary>
        double horizontalFieldOfView;

        /// <summary>
        /// The vertical field-of-view angle in radians.
        /// </summary>
        double verticalFieldOfView;

        // TODO: Add support for orthographic and off-center perspective frustums
    };

}
