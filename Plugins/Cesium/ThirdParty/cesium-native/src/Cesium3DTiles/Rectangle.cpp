#include "Cesium3DTiles/Rectangle.h"
#include "CesiumUtility/Math.h"

namespace Cesium3DTiles {

    Rectangle::Rectangle(
        double west,
        double south,
        double east,
        double north
    ) :
        _west(west),
        _south(south),
        _east(east),
        _north(north)
    {
    }

    double Rectangle::computeWidth() const {
        double east = this->_east;
        double west = this->_west;
        if (east < west) {
            east += Math::TWO_PI;
        }
        return east - west;
    }

    double Rectangle::computeHeight() const {
        return this->_north - this->_south;
    }

    Cartographic Rectangle::computeCenter() const {
        double east = this->_east;
        double west = this->_west;

        if (east < west) {
            east += Math::TWO_PI;
        }

        double longitude = Math::negativePiToPi((west + east) * 0.5);
        double latitude = (this->_south + this->_north) * 0.5;

        return Cartographic(longitude, latitude, 0.0);
    }

    bool Rectangle::contains(const Cartographic& cartographic) const {
        double longitude = cartographic.longitude;
        double latitude = cartographic.latitude;

        double west = this->_west;
        double east = this->_east;

        if (east < west) {
            east += Math::TWO_PI;
            if (longitude < 0.0) {
                longitude += Math::TWO_PI;
            }
        }
        return (
            (longitude > west || Math::equalsEpsilon(longitude, west, Math::EPSILON14)) &&
            (longitude < east || Math::equalsEpsilon(longitude, east, Math::EPSILON14)) &&
            latitude >= this->_south &&
            latitude <= this->_north
        );
    }
}
