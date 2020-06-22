#pragma once

#include <functional>

class IAssetAccessor;
class Cesium3DTile;

class IPrepareRendererResources {
public:
    /// <summary>
    /// Prepares renderer resources for the given tile. The preparation may happen synchronously
    /// (i.e. before this method returns) or asynchronously. In either case, call
    /// <see cref="Cesium3DTile.finishPrepareRendererResources" /> when the preparation is
    /// complete.
    /// </summary>
    /// <param name="tile">The tile for which to prepare resources.</param>
    virtual void prepare(Cesium3DTile& tile) = 0;

    /// <summary>
    /// Cancels asynchronous preparation of renderer resources for the given tile. Upon return
    /// from this method, the tile and any of its sub-objects may be deleted at any time and
    /// must not be used.
    /// </summary>
    /// <param name="tile">The tile for which to cancel preparation.</param>
    virtual void cancel(Cesium3DTile& tile) = 0;

    /// <summary>
    /// Frees the renderer resources associated with the given tile.
    /// </summary>
    /// <param name="tile">The tile.</param>
    /// <param name="pRendererResources">The renderer resources pointer, if one was provided.</param>
    virtual void free(Cesium3DTile& tile, void* pRendererResources) = 0;
};

class ITaskProcessor {
public:
    virtual void startTask(std::function<void ()> f) = 0;
};

class Cesium3DTilesetExternals {
public:
    IAssetAccessor* pAssetAccessor;
    IPrepareRendererResources* pPrepareRendererResources;
    ITaskProcessor* pTaskProcessor;
};
