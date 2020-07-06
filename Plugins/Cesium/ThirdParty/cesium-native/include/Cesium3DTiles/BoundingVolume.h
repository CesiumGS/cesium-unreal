#pragma once

#include <variant>
#include "BoundingBox.h"
#include "BoundingRegion.h"
#include "BoundingSphere.h"

namespace Cesium3DTiles {
    typedef std::variant<BoundingBox, BoundingRegion, BoundingSphere> BoundingVolume;

    BoundingVolume transformBoundingVolume(const glm::dmat4x4& transform, const BoundingVolume& boundingVolume);
    glm::dvec3 getBoundingVolumeCenter(const BoundingVolume& boundingVolume);
}
