#pragma once

#include <optional>
#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <glm/mat4x4.hpp>
#include "IAssetRequest.h"
#include "TileContent.h"
#include "VectorReference.h"
#include "VectorRange.h"
#include "BoundingVolume.h"

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

        enum Refine {
            Add = 0,
            Replace = 1
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

        const BoundingVolume& getBoundingVolume() const { return this->_boundingVolume; }
        void setBoundingVolume(const BoundingVolume& value) { this->_boundingVolume = value; }

        const std::optional<BoundingVolume>& getViewerRequestVolume() const { return this->_viewerRequestVolume; }
        void setViewerRequestVolume(const std::optional<BoundingVolume>& value) { this->_viewerRequestVolume = value; }

        double getGeometricError() const { return this->_geometricError; }
        void setGeometricError(double value) { this->_geometricError = value; }

        const std::optional<Refine>& getRefine() const { return this->_refine; }
        void setRefine(const std::optional<Refine>& value) { this->_refine = value; }

        const std::optional<glm::dmat4x4>& getTransform() const { return this->_transform; }
        void setTransform(const std::optional<glm::dmat4x4>& value) { this->_transform = value; }

        const std::optional<std::string>& getContentUri() const { return this->_contentUri; }
        void setContentUri(const std::optional<std::string>& value);

        const std::optional<BoundingVolume>& getContentBoundingVolume() const { return this->_contentBoundingVolume; }
        void setContentBoundingVolume(const std::optional<BoundingVolume>& value) { this->_contentBoundingVolume = value; }

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
        BoundingVolume _boundingVolume;
        std::optional<BoundingVolume> _viewerRequestVolume;
        double _geometricError;
        std::optional<Refine> _refine;
        std::optional<glm::dmat4x4> _transform;

        std::optional<std::string> _contentUri;
        std::optional<BoundingVolume> _contentBoundingVolume;

        // Load state and data.
        std::atomic<LoadState> _state;
        std::unique_ptr<IAssetRequest> _pContentRequest;
        std::unique_ptr<TileContent> _pContent;
        void* _pRendererResources;
    };

}
