#include "Cesium3DTiles/BoundingRegion.h"

namespace Cesium3DTiles {

    BoundingRegion::BoundingRegion(
        double west,
        double south,
        double east,
        double north,
        double minimumHeight,
        double maximumHeight
    ) :
        _west(west),
        _south(south),
        _east(east),
        _north(north),
        _minimumHeight(minimumHeight),
        _maximumHeight(maximumHeight),
        _boundingBox(BoundingBox::fromRectangle(Rectangle(west, south, east, north), minimumHeight, maximumHeight))
    {
    }

    CullingResult BoundingRegion::intersectPlane(const Plane& plane) const {
        return CullingResult::Inside;
    }

}
