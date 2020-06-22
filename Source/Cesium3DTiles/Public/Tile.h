#pragma once

#include <optional>
#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <gsl/span>
#include "IAssetRequest.h"
#include "TileContent.h"
#include "VectorReference.h"
#include "VectorRange.h"


namespace Cesium3DTiles {
    class Tileset;
    class TileContent;

    class CESIUM3DTILES_API Tile {
    public:
        enum LoadState {
            /// <summary>
            /// Something went wrong while loading this tile.
            /// </summary>
            Failed = -1,

            /// <summary>
            /// The tile is not yet loaded at all, beyond the metadata in tileset.json.
            /// </summary>
            Unloaded = 0,

            /// <summary>
            /// The tile content is currently being loaded.
            /// </summary>
            ContentLoading = 1,

            /// <summary>
            /// The tile content has finished loading.
            /// </summary>
            ContentLoaded = 2,

            /// <summary>
            /// The tile's renderer resources are currently being prepared.
            /// </summary>
            RendererResourcesPreparing = 3,

            /// <summary>
            /// The tile's renderer resources are done being prepared and this
            /// tile is ready to render.
            /// </summary>
            RendererResourcesPrepared = 4
        };

        Tile(const Cesium3DTiles::Tileset& tileset, VectorReference<Tile> pParent = VectorReference<Tile>());
        ~Tile();
        Tile(Tile& rhs) noexcept = delete;
        Tile(Tile&& rhs) noexcept;
        Tile& operator=(Tile&& rhs) noexcept;

        Tile* getParent() { return &*this->_pParent.data(); }
        const Tile* getParent() const { return this->_pParent.data(); }

        VectorRange<Tile>& getChildren() { return this->_children; }
        const VectorRange<Tile>& getChildren() const { return this->_children; }
        void setChildren(const VectorRange<Tile>& children);

        const std::optional<std::string>& getContentUri() const { return this->_contentUri; }
        void setContentUri(const std::optional<std::string>& value);

        TileContent* getContent() { return this->_pContent.get(); }
        const TileContent* getContent() const { return this->_pContent.get(); }

        void* getRendererResources() { return this->_pRendererResources; }

        LoadState getState() const { return this->_state.load(std::memory_order::memory_order_acquire); }

        void loadContent();

        /// <summary>
        /// Notifies the tile that its renderer resources have been prepared and optionally stores
        /// a pointer to those resources. This method is safe to call from any thread.
        /// </summary>
        /// <param name="pResource">The renderer resources as an opaque pointer.</param>
        void finishPrepareRendererResources(void* pResource = nullptr);

    protected:
        void setState(LoadState value);
        void contentResponseReceived(IAssetRequest* pRequest);

    private:
        // Position in bounding-volume hierarchy.
        const Cesium3DTiles::Tileset* _pTileset;
        VectorReference<Tile> _pParent;
        VectorRange<Tile> _children;

        // Properties from tileset.json.
        // These are immutable after the tile leaves TileState::Unloaded.
        std::optional<std::string> _contentUri;

        // Load state and data.
        std::atomic<LoadState> _state;
        std::unique_ptr<IAssetRequest> _pContentRequest;
        std::unique_ptr<TileContent> _pContent;
        void* _pRendererResources;
    };

}