#pragma once

#include <glm/mat4x4.hpp>

class CesiumTransforms {
public:
    static const double centimetersPerMeter;

    // Scale Cesium's meters up to Unreal's centimeters.
    static const glm::dmat4x4 scaleToUnrealWorld;

    // Transform Cesium's right-handed, Z-up coordinate system to Unreal's left-handed, Z-up coordinate
    // system by inverting the Y coordinate. This same transformation can also go the other way.
    static const glm::dmat4x4 unrealToOrFromCesium;
};