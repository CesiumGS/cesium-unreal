#include "Cesium3DTiles/BoundingRegion.h"

namespace Cesium3DTiles {

    CullingResult BoundingRegion::intersectPlane(const Plane& plane) const {
        return CullingResult::Inside;
    }

}
