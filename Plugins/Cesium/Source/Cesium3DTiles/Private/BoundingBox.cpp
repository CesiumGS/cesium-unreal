#include "BoundingBox.h"
#include "Plane.h"

namespace Cesium3DTiles {

    CullingResult BoundingBox::intersectPlane(const Plane& plane) const {
        return CullingResult::Inside;
    }

}
