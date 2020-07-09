#pragma once

#include <functional>

namespace Cesium3DTiles {

    class IAssetResponse;

    /// <summary>
    /// An asynchronous request for a 3D Tiles asset.
    /// </summary>
    class IAssetRequest {
    public:
        virtual ~IAssetRequest() = default;

        /// <summary>
        /// Gets the response, or nullptr is the request is still in progress.
        /// </summary>
        /// <returns>The response.</returns>
        virtual IAssetResponse* response() = 0;

        /// <summary>
        /// Binds a callback function that will be invoked when the request's response is received.
        /// </summary>
        /// <param name="callback">The callback.</param>
        virtual void bind(std::function<void(IAssetRequest*)> callback) = 0;

        /// <summary>
        /// Gets the requested URL.
        /// </summary>
        virtual std::string url() const = 0;

        /// <summary>
        /// Cancels the request.
        /// </summary>
        virtual void cancel() = 0;
    };

}
