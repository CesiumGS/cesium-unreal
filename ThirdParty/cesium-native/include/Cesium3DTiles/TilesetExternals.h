#pragma once

#include <functional>

namespace Cesium3DTiles {
    class IAssetAccessor;
    class Tile;

    class IPrepareRendererResources {
    public:
        /**
         * Prepares renderer resources for the given tile. This method is invoked in the load
         * thread, and it may not modify the tile.
         * @param tile The tile to prepare.
         * @returns Arbitrary data representing the result of the load process. This data is
         *          passed to \ref prepareInMainThread as the `pLoadThreadResult` parameter.
         */
        virtual void* prepareInLoadThread(const Tile& tile) = 0;

        /**
         * Further prepares renderer resources. This is called after \ref prepareInLoadThread,
         * and unlike that method, this one is called from the same thread that called \ref Tileset::updateView.
         * @param tile The tile to prepare.
         * @param pLoadThreadResult The value returned from \ref prepareInLoadThread.
         * @returns Arbitrary data representing the result of the load process. Note that the value returned
         *          by \ref prepareInLoadThread will _not_ be automatically preserved and passed to
         *          \ref free. If you need to free that value, do it in this method before returning. If you need
         *          that value later, add it to the object returned from this method.
         */
        virtual void* prepareInMainThread(Tile& tile, void* pLoadThreadResult) = 0;

        /**
         * Frees previously-prepared renderer resources. This method is always called from
         * the thread that called \ref Tileset::updateView or deleted the tileset.
         * @param tile The tile for which to free renderer resources.
         * @param pLoadThreadResult The result returned by \ref prepareInLoadThread. If \ref prepareInMainThread
         *        has already been called, this parameter will be nullptr.
         * @param pMainThreadResult The result returned by \ref prepareInMainThread. If \ref prepareInMainThread
         *        has not yet been called, this parameter will be nullptr.
         */
        virtual void free(Tile& tile, void* pLoadThreadResult, void* pMainThreadResult) = 0;        
    };

    class ITaskProcessor {
    public:
        virtual void startTask(std::function<void()> f) = 0;
    };

    class TilesetExternals {
    public:
        IAssetAccessor* pAssetAccessor;
        IPrepareRendererResources* pPrepareRendererResources;
        ITaskProcessor* pTaskProcessor;
    };

}
