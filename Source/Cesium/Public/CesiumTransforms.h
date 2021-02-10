#pragma once

#include <glm/mat4x4.hpp>

class CesiumTransforms {
public:
    static const double metersToCentimeters;
    static const double centimetersToMeters;

    // Scale Cesium's meters up to Unreal's centimeters.
    static const glm::dmat4x4 scaleToUnrealWorld;

    // Scale down Unreal's centimeters into Cesium's meters.
    static const glm::dmat4x4 scaleToCesium;

    // Transform Cesium's right-handed, Z-up coordinate system to Unreal's left-handed, Z-up coordinate
    // system by inverting the Y coordinate. This same transformation can also go the other way.
    static const glm::dmat4x4 unrealToOrFromCesium;
};