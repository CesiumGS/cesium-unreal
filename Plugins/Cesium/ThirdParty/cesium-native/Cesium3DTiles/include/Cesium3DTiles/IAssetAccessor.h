#pragma once

#include <string>
#include "Cesium3DTiles/Library.h"
#include "IAssetRequest.h"

namespace Cesium3DTiles {

    /// <summary>
    /// Provides asynchronous access to 3D Tiles assets like tileset.json and tile content.
    /// </summary>
    class CESIUM3DTILES_API IAssetAccessor {
    public:
        virtual ~IAssetAccessor() = default;

        /// <summary>
        /// Starts a new request for the asset with the given URL. The request proceeds asynchronously
        /// without blocking the calling thread.
        /// </summary>
        /// <param name="url">The URL of the asset.</param>
        /// <returns>The in-progress asset request.</returns>
        virtual std::unique_ptr<IAssetRequest> requestAsset(const std::string& url) = 0;

        /**
         * Ticks the asset accessor system while the main thread is blocked.
         * If the asset accessor is not dependent on the main thread to
         * dispatch requests, this method does not need to do anything.
         */
        virtual void tick() = 0;
    };

}
