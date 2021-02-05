#include "CesiumTransforms.h"

const double CesiumTransforms::centimetersPerMeter = 100.0;

// Scale Cesium's meters up to Unreal's centimeters.
const glm::dmat4x4 CesiumTransforms::scaleToUnrealWorld = glm::dmat4x4(glm::dmat3x3(centimetersPerMeter));

// Scale down Unreal's centimeters into Cesium's meters.
const glm::dmat4x4 CesiumTransforms::scaleToCesium = glm::dmat4x4(glm::dmat3x3(1.0 / centimetersPerMeter));

// We're initializing with a static function instead of inline to avoid an
// internal compiler error in MSVC v14.27.29110.
static glm::dmat4 createUnrealToOrFromCesium() {
    return glm::dmat4x4(
        glm::dvec4(1.0, 0.0, 0.0, 0.0),
        glm::dvec4(0.0, -1.0, 0.0, 0.0),
        glm::dvec4(0.0, 0.0, 1.0, 0.0),
        glm::dvec4(0.0, 0.0, 0.0, 1.0)
    );
}

// Transform Cesium's right-handed, Z-up coordinate system to Unreal's left-handed, Z-up coordinate
// system by inverting the Y coordinate. This same transformation can also go the other way.
const glm::dmat4x4 CesiumTransforms::unrealToOrFromCesium = createUnrealToOrFromCesium();