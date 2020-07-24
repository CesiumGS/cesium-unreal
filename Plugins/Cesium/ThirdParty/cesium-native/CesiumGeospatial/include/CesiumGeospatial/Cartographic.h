#pragma once

namespace Cesium3DTiles {

    class Cartographic {
    public:
        Cartographic(double longitude, double latitude, double height = 0.0);

        double longitude;
        double latitude;
        double height;
    };
}
