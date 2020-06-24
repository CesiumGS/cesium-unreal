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

        this->_updateCullingVolume();
    }

    void Camera::updateViewParameters(const glm::dvec2& viewportSize, double horizontalFieldOfView, double verticalFieldOfView) {
        this->_viewportSize = viewportSize;
        this->_horizontalFieldOfView = horizontalFieldOfView;
        this->_verticalFieldOfView = verticalFieldOfView;

        this->_sseDenominator = 2.0 * tan(0.5 * this->_verticalFieldOfView);

        this->_updateCullingVolume();
    }

    void Camera::_updateCullingVolume() {
        double aspectRatio = this->_viewportSize.y / this->_viewportSize.x;
        double fov = this->_horizontalFieldOfView;
        double fovy = this->_verticalFieldOfView;

        double t = tan(0.5 * fovy);
        double b = -t;
        double r = aspectRatio * t;
        double l = -r;

        double n = 1.0;

        // TODO: this is all ported directly from CesiumJS, can probably be refactored to be more efficient with GLM.

        glm::dvec3 right = glm::cross(this->_direction, this->_up);

        glm::dvec3 nearCenter = this->_position + this->_direction;

        //Left plane computation
        glm::dvec3 normal = right * l;
        normal = nearCenter + normal;
        normal = normal - this->_position;
        glm::normalize(normal);
        normal = glm::cross(normal, this->_up);
        glm::normalize(normal);

        this->_leftPlane = Plane(glm::dvec4(
            normal,
            -glm::dot(normal, this->_position)
        ));

        //Right plane computation
        normal = right * r;
        normal = nearCenter + normal;
        normal = normal - this->_position;
        normal = glm::cross(this->_up, normal);
        normal = normalize(normal);

        this->_rightPlane = Plane(glm::dvec4(
            normal,
            -glm::dot(normal, this->_position)
        ));

        //Bottom plane computation
        normal = this->_up * b;
        normal = nearCenter + normal;
        normal = normal - this->_position;
        normal = glm::cross(right, normal);
        normal = glm::normalize(normal);

        this->_bottomPlane = Plane(glm::dvec4(
            normal,
            -glm::dot(normal, this->_position)
        ));

        //Top plane computation
        normal = this->_up * t;
        normal = nearCenter + normal;
        normal = normal - this->_position;
        normal = glm::cross(normal, right);
        normal = glm::normalize(normal);

        this->_topPlane = Plane(glm::dvec4(
            normal,
            -glm::dot(normal, this->_position)
        ));
    }

    template <class T>
    static bool isBoundingVolumeVisible(
        const T& boundingVolume,
        const Plane& leftPlane,
        const Plane& rightPlane,
        const Plane& bottomPlane,
        const Plane& topPlane
    ) {
        CullingResult left = boundingVolume.intersectPlane(leftPlane);
        if (left == CullingResult::Outside) {
            return false;
        }

        CullingResult right = boundingVolume.intersectPlane(rightPlane);
        if (right == CullingResult::Outside) {
            return false;
        }

        CullingResult top = boundingVolume.intersectPlane(topPlane);
        if (top == CullingResult::Outside) {
            return false;
        }

        CullingResult bottom = boundingVolume.intersectPlane(bottomPlane);
        if (bottom == CullingResult::Outside) {
            return false;
        }

        return true;
    }

    bool Camera::isBoundingVolumeVisible(const BoundingVolume& boundingVolume) const {
        // TODO: use plane masks
        switch (boundingVolume.index()) {
        case 0:
        {
            const BoundingBox& boundingBox = std::get<BoundingBox>(boundingVolume);
            return Cesium3DTiles::isBoundingVolumeVisible(boundingBox, this->_leftPlane, this->_rightPlane, this->_bottomPlane, this->_topPlane);
        }
        case 1:
        {
            const BoundingRegion& boundingRegion = std::get<BoundingRegion>(boundingVolume);
            return Cesium3DTiles::isBoundingVolumeVisible(boundingRegion, this->_leftPlane, this->_rightPlane, this->_bottomPlane, this->_topPlane);
        }
        case 2:
        {
            const BoundingSphere& boundingSphere = std::get<BoundingSphere>(boundingVolume);
            return Cesium3DTiles::isBoundingVolumeVisible(boundingSphere, this->_leftPlane, this->_rightPlane, this->_bottomPlane, this->_topPlane);
        }
        default:
            return true;
        }
    }

    double Camera::computeDistanceToBoundingVolume(const BoundingVolume& boundingVolume) const {
        return 0.0;
    }

    double Camera::computeScreenSpaceError(double geometricError, double distance) const {
        return 9999999999.0;
    }
}
