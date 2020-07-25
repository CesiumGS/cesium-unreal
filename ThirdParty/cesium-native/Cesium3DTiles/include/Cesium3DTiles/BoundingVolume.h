#pragma once

#include <variant>
#include "Cesium3DTiles/Library.h"
#include "CesiumGeometry/OrientedBoundingBox.h"
#include "CesiumGeospatial/BoundingRegion.h"
#include "CesiumGeometry/BoundingSphere.h"

namespace Cesium3DTiles {
    typedef std::variant<CesiumGeometry::OrientedBoundingBox, CesiumGeospatial::BoundingRegion, CesiumGeometry::BoundingSphere> BoundingVolume;

    CESIUM3DTILES_API BoundingVolume transformBoundingVolume(const glm::dmat4x4& transform, const BoundingVolume& boundingVolume);
    CESIUM3DTILES_API glm::dvec3 getBoundingVolumeCenter(const BoundingVolume& boundingVolume);
}
