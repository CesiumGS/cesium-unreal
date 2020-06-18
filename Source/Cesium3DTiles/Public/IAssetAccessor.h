#pragma once

#include <string>
#include "IAssetRequest.h"

/// <summary>
/// Provides asynchronous access to 3D Tiles assets like tileset.json and tile content.
/// </summary>
class IAssetAccessor {
public:
    virtual ~IAssetAccessor() = default;

    /// <summary>
    /// Starts a new request for the asset with the given URL. The request proceeds asynchronously
    /// without blocking the calling thread.
    /// </summary>
    /// <param name="url">The URL of the asset.</param>
    /// <returns>The in-progress asset request.</returns>
    virtual std::unique_ptr<IAssetRequest> requestAsset(const std::string& url) = 0;
};
