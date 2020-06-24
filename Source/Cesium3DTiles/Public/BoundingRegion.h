#pragma once

#include "CullingResult.h"

namespace Cesium3DTiles {

    class Plane;

    class BoundingRegion {
    public:
        BoundingRegion() = default;
        BoundingRegion(
            double west,
            double south,
            double east,
            double north,
            double minimumHeight,
            double maximumHeight
        ) :
            west(west),
            south(south),
            east(east),
            north(north),
            minimumHeight(minimumHeight),
            maximumHeight(maximumHeight)
        {
        }

        CullingResult intersectPlane(const Plane& plane) const;
        
        double west;
        double south;
        double east;
        double north;
        double minimumHeight;
        double maximumHeight;
    };

}
