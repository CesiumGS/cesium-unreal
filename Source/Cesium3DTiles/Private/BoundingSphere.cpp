#include "BoundingSphere.h"

namespace Cesium3DTiles {

    CullingResult BoundingSphere::intersectPlane(const Plane& plane) const {
        return CullingResult::Inside;
    }

}
