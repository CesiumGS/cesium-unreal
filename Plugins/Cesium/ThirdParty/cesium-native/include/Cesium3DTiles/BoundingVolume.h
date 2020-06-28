#pragma once

#include <variant>
#include "BoundingBox.h"
#include "BoundingRegion.h"
#include "BoundingSphere.h"

namespace Cesium3DTiles {
    typedef std::variant<BoundingBox, BoundingRegion, BoundingSphere> BoundingVolume;
}
