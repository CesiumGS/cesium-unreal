#include "CesiumGeospatial/Cartographic.h"

namespace CesiumGeospatial {

    Cartographic::Cartographic(double longitudeParam, double latitudeParam, double heightParam) :
        longitude(longitudeParam),
        latitude(latitudeParam),
        height(heightParam)
    {
    }

}
