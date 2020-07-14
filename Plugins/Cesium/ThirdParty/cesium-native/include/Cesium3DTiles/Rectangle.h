#pragma once

#include "Cesium3DTiles/Cartographic.h"

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

        double computeWidth() const;
        double computeHeight() const;
        Cartographic computeCenter() const;

    private:
        double _west;
        double _south;
        double _east;
        double _north;
    };
}
