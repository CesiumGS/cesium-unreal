#pragma once

namespace Cesium3DTiles {

    enum class CullingResult {
        /// <summary>
        /// Indicates that an object lies completely outside the culling volume.
        /// </summary>
        Outside = -1,

        /// <summary>
        /// Indicates that an object intersects with the boundary of the culling volume,
        /// so the object is partially inside and partially outside the culling volume.
        /// </summary>
        Intersecting = 0,

        /// <summary>
        /// Indicates that an object lies completely inside the culling volume.
        /// </summary>
        Inside = 1
    };

}
