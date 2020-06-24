#include "Camera.h"

namespace Cesium3DTiles {

    Camera::Camera(
        const glm::dvec3& position,
        const glm::dvec3& direction,
        const glm::dvec3& up,
        const glm::dvec2& viewportSize,
        double horizontalFieldOfView,
        double verticalFieldOfView
    ) :
        _position(position),
        _direction(direction),
        _up(up),
        _viewportSize(viewportSize),
        _horizontalFieldOfView(horizontalFieldOfView),
        _verticalFieldOfView(verticalFieldOfView)
    {
        this->updatePositionAndOrientation(position, direction, up);
        this->updateViewParameters(viewportSize, horizontalFieldOfView, verticalFieldOfView);
    }

    void Camera::updatePositionAndOrientation(const glm::dvec3& position, const glm::dvec3& direction, const glm::dvec3& up) {
        this->_position = position;
        this->_direction = direction;
        this->_up = up;
    }

    void Camera::updateViewParameters(const glm::dvec2& viewportSize, double horizontalFieldOfView, double verticalFieldOfView) {
        this->_viewportSize = viewportSize;
        this->_horizontalFieldOfView = horizontalFieldOfView;
        this->_verticalFieldOfView = verticalFieldOfView;

        this->_sseDenominator = 2.0 * tan(0.5 * this->_verticalFieldOfView);
    }

    bool Camera::isBoundingVolumeVisible(const BoundingVolume& boundingVolume) const {
        // TODO: use plane masks
        switch (boundingVolume.index()) {
        case 0:
        {
            const BoundingBox& boundingBox = std::get<BoundingBox>(boundingVolume);
            return this->isBoundingVolumeVisible(boundingBox);
        }
        case 1:
        {
            const BoundingRegion& boundingRegion = std::get<BoundingRegion>(boundingVolume);
            return this->isBoundingVolumeVisible(boundingRegion);
        }
        case 2:
        {
            const BoundingSphere& boundingSphere = std::get<BoundingSphere>(boundingVolume);
            return this->isBoundingVolumeVisible(boundingSphere);
        }
        default:
            return true;
        }
    }

    bool Camera::isBoundingVolumeVisible(const BoundingBox& boundingVolume) const {
        return true;
    }

    bool Camera::isBoundingVolumeVisible(const BoundingRegion& boundingRegion) const {
        return true;
    }

    bool Camera::isBoundingVolumeVisible(const BoundingSphere& boundingSphere) const {
        return true;
    }

    double Camera::computeDistanceToBoundingVolume(const BoundingVolume& boundingVolume) const {
        return 0.0;
    }

    double Camera::computeScreenSpaceError(double geometricError, double distance) const {
        return 9999999999.0;
    }
}
