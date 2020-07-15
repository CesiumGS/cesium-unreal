#pragma once

#include "Cesium3DTiles/CullingResult.h"
#include "Cesium3DTiles/BoundingBox.h"

namespace Cesium3DTiles {

    class Plane;

    class BoundingRegion {
    public:
        BoundingRegion(
            double west,
            double south,
            double east,
            double north,
            double minimumHeight,
            double maximumHeight
        );

        double getWest() const { return this->_west; }
        double getSouth() const { return this->_south; }
        double getEast() const { return this->_east; }
        double getNorth() const { return this->_north; }
        double getMinimumHeight() const { return this->_minimumHeight; }
        double getMaximumHeight() const { return this->_maximumHeight; }
        const BoundingBox& getBoundingBox() const { return this->_boundingBox; }

        CullingResult intersectPlane(const Plane& plane) const;
        
private:
        double _west;
        double _south;
        double _east;
        double _north;
        double _minimumHeight;
        double _maximumHeight;
        BoundingBox _boundingBox;
    };

}
