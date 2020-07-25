#pragma once

#include "CesiumGeospatial/Library.h"

namespace CesiumGeospatial {

    class CESIUMGEOSPATIAL_API Cartographic {
    public:
        Cartographic(double longitude, double latitude, double height = 0.0);

        double longitude;
        double latitude;
        double height;
    };
}
