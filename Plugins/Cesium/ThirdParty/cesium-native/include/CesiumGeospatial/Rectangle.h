#pragma once

#include "CesiumGeospatial/Cartographic.h"

namespace Cesium3DTiles {

    class Rectangle {
    public:
        Rectangle(
            double west,
            double south,
            double east,
            double north
        );

        double getWest() const { return this->_west; }
        double getSouth() const { return this->_south; }
        double getEast() const { return this->_east; }
        double getNorth() const { return this->_north; }
        Cartographic getSouthwest() const { return Cartographic(this->_west, this->_south); }
        Cartographic getSoutheast() const { return Cartographic(this->_east, this->_south); }
        Cartographic getNorthwest() const { return Cartographic(this->_west, this->_north); }
        Cartographic getNortheast() const { return Cartographic(this->_east, this->_north); }

        double computeWidth() const;
        double computeHeight() const;
        Cartographic computeCenter() const;

        bool contains(const Cartographic& cartographic) const;

    private:
        double _west;
        double _south;
        double _east;
        double _north;
    };
}
