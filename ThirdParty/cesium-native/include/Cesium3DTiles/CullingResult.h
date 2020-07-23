#pragma once

namespace Cesium3DTiles {

    /**
     * The result of culling an object.
     */
    enum class CullingResult {
        /**
         * Indicates that an object lies completely outside the culling volume.
         */
        Outside = -1,

        /**
         * Indicates that an object intersects with the boundary of the culling volume,
         * so the object is partially inside and partially outside the culling volume.
         */
        Intersecting = 0,

        /**
         * Indicates that an object lies completely inside the culling volume.
         */
        Inside = 1
    };

}
