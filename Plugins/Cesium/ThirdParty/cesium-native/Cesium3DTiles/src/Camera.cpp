#include "Cesium3DTiles/Camera.h"
#include <algorithm>

using namespace CesiumGeometry;
using namespace CesiumGeospatial;

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
        _verticalFieldOfView(verticalFieldOfView),
        _sseDenominator(0.0),
        _leftPlane(glm::dvec3(0.0, 0.0, 1.0), 0.0),
        _rightPlane(glm::dvec3(0.0, 0.0, 1.0), 0.0),
        _topPlane(glm::dvec3(0.0, 0.0, 1.0), 0.0),
        _bottomPlane(glm::dvec3(0.0, 0.0, 1.0), 0.0)
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
        double fovx = this->_horizontalFieldOfView;
        double fovy = this->_verticalFieldOfView;

        double t = tan(0.5 * fovy);
        double b = -t;
        double r = tan(0.5 * fovx);
        double l = -r;

        double n = 1.0;

        // TODO: this is all ported directly from CesiumJS, can probably be refactored to be more efficient with GLM.

        glm::dvec3 right = glm::cross(this->_direction, this->_up);

        glm::dvec3 nearCenter = this->_direction * n;
        nearCenter = this->_position + nearCenter;

        //Left plane computation
        glm::dvec3 normal = right * l;
        normal = nearCenter + normal;
        normal = normal - this->_position;
        glm::normalize(normal);
        normal = glm::cross(normal, this->_up);
        glm::normalize(normal);

        this->_leftPlane = Plane(
            normal,
            -glm::dot(normal, this->_position)
        );

        //Right plane computation
        normal = right * r;
        normal = nearCenter + normal;
        normal = normal - this->_position;
        normal = glm::cross(this->_up, normal);
        normal = normalize(normal);

        this->_rightPlane = Plane(
            normal,
            -glm::dot(normal, this->_position)
        );

        //Bottom plane computation
        normal = this->_up * b;
        normal = nearCenter + normal;
        normal = normal - this->_position;
        normal = glm::cross(right, normal);
        normal = glm::normalize(normal);

        this->_bottomPlane = Plane(
            normal,
            -glm::dot(normal, this->_position)
        );

        //Top plane computation
        normal = this->_up * t;
        normal = nearCenter + normal;
        normal = normal - this->_position;
        normal = glm::cross(normal, right);
        normal = glm::normalize(normal);

        this->_topPlane = Plane(
            normal,
            -glm::dot(normal, this->_position)
        );
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
            const OrientedBoundingBox& boundingBox = std::get<OrientedBoundingBox>(boundingVolume);
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

    double Camera::computeDistanceSquaredToBoundingVolume(const BoundingVolume& boundingVolume) const {
        switch (boundingVolume.index()) {
        case 0:
        {
            const OrientedBoundingBox& boundingBox = std::get<OrientedBoundingBox>(boundingVolume);
            return boundingBox.computeDistanceSquaredToPosition(this->_position);
        }
        case 1:
        {
            const BoundingRegion& boundingRegion = std::get<BoundingRegion>(boundingVolume);
            return boundingRegion.computeDistanceSquaredToPosition(this->_position);
        }
        case 2:
        {
            const BoundingSphere& boundingSphere = std::get<BoundingSphere>(boundingVolume);
            return boundingSphere.computeDistanceSquaredToPosition(this->_position);
        }
        default:
            return 0.0;
        }
    }

    double Camera::computeScreenSpaceError(double geometricError, double distance) const {
        // Avoid divide by zero when viewer is inside the tile
        distance = std::max(distance, 1e-7);
        double sseDenominator = this->_sseDenominator;
        return (geometricError * this->_viewportSize.y) / (distance * sseDenominator);
    }
}
