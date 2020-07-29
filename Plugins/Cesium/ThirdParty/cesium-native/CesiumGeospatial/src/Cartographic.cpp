#include "CesiumGeospatial/Cartographic.h"
#include "CesiumUtility/Math.h"

using namespace CesiumUtility;

namespace CesiumGeospatial {

    Cartographic::Cartographic(double longitudeParam, double latitudeParam, double heightParam) :
        longitude(longitudeParam),
        latitude(latitudeParam),
        height(heightParam)
    {
    }

    /*static*/ Cartographic Cartographic::fromDegrees(double longitudeDegrees, double latitudeDegrees, double heightMeters) {
        return Cartographic(
            Math::degreesToRadians(longitudeDegrees),
            Math::degreesToRadians(latitudeDegrees),
            heightMeters
        );
    }
}
