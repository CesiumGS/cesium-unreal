#pragma once

namespace Cesium3DTiles {

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
        
        double west;
        double south;
        double east;
        double north;
        double minimumHeight;
        double maximumHeight;
    };

}
