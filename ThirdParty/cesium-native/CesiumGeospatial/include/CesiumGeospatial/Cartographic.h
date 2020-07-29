#pragma once

#include "CesiumGeospatial/Library.h"

namespace CesiumGeospatial {

    class CESIUMGEOSPATIAL_API Cartographic {
    public:
        Cartographic(double longitudeRadians, double latitudeRadians, double heightMeters = 0.0);

        static Cartographic fromDegrees(double longitudeDegrees, double latitudeDegrees, double heightMeters = 0.0);

        double longitude;
        double latitude;
        double height;
    };
}
