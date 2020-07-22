#include "Cesium3DTiles/Tile.h"
#include "Cesium3DTiles/Tileset.h"
#include "Cesium3DTiles/IAssetAccessor.h"
#include "Cesium3DTiles/IAssetResponse.h"
#include "Cesium3DTiles/TileContentFactory.h"
#include <chrono>

namespace Cesium3DTiles {

    Tile::Tile() :
        _loadedTilesLinks(),
        _pTileset(nullptr),
        _pParent(nullptr),
        _children(),
        _boundingVolume(BoundingBox(glm::dvec3(), glm::dmat4())),
        _viewerRequestVolume(),
        _geometricError(0.0),
        _refine(),
        _transform(1.0),
        _contentUri(),
        _contentBoundingVolume(),
        _state(LoadState::Unloaded),
        _pContentRequest(nullptr),
        _pContent(nullptr),
        _pRendererResources(nullptr),
        _lastSelectionState()
    {
    }

    Tile::~Tile() {
        this->prepareToDestroy();

        // Wait for this tile to exit the "Destroying" state, indicating that
        // work happening in the loading thread has concluded.
        if (this->getState() == LoadState::Destroying) {
            const auto timeoutSeconds = std::chrono::seconds(5LL);

            auto start = std::chrono::steady_clock::now();
            while (this->getState() == LoadState::Destroying) {
                auto duration = std::chrono::steady_clock::now() - start;
                if (duration > timeoutSeconds) {
                    // TODO: report timeout, because it may be followed by a crash.
                    return;
                }
                this->_pTileset->externals().pAssetAccessor->tick();
            }
        }

        this->unloadContent();
    }

    Tile::Tile(Tile&& rhs) noexcept :
        _loadedTilesLinks(std::move(rhs._loadedTilesLinks)),
        _pTileset(rhs._pTileset),
        _pParent(rhs._pParent),
        _children(std::move(rhs._children)),
        _boundingVolume(rhs._boundingVolume),
        _viewerRequestVolume(rhs._viewerRequestVolume),
        _geometricError(rhs._geometricError),
        _refine(rhs._refine),
        _transform(rhs._transform),
        _contentUri(rhs._contentUri),
        _contentBoundingVolume(rhs._contentBoundingVolume),
        _state(rhs.getState()),
        _pContentRequest(std::move(rhs._pContentRequest)),
        _pContent(std::move(rhs._pContent)),
        _pRendererResources(rhs._pRendererResources),
        _lastSelectionState(rhs._lastSelectionState)
    {
    }

    Tile& Tile::operator=(Tile&& rhs) noexcept {
        if (this != &rhs) {
            this->_loadedTilesLinks = std::move(rhs._loadedTilesLinks);
            this->_pTileset = rhs._pTileset;
            this->_pParent = rhs._pParent;
            this->_children = std::move(rhs._children);
            this->_boundingVolume = rhs._boundingVolume;
            this->_viewerRequestVolume = rhs._viewerRequestVolume;
            this->_geometricError = rhs._geometricError;
            this->_refine = rhs._refine;
            this->_transform = rhs._transform;
            this->_contentUri = rhs._contentUri;
            this->_contentBoundingVolume = rhs._contentBoundingVolume;
            this->setState(rhs.getState());
            this->_pContentRequest = std::move(rhs._pContentRequest);
            this->_pContent = std::move(rhs._pContent);
            this->_pRendererResources = rhs._pRendererResources;
            this->_lastSelectionState = rhs._lastSelectionState;
        }

        return *this;
    }

    void Tile::prepareToDestroy() {
        if (this->_pContentRequest) {
            this->_pContentRequest->cancel();
        }

        // Atomically change any tile in the ContentLoading state to the Destroying state.
        // Tiles in other states remain in those states.
        LoadState stateToChange = LoadState::ContentLoading;
        this->_state.compare_exchange_strong(stateToChange, LoadState::Destroying);
    }

    void Tile::createChildTiles(size_t count) {
        if (this->_children.size() > 0) {
            throw std::runtime_error("Children already created.");
        }
        this->_children.resize(count);
    }

    void Tile::createChildTiles(std::vector<Tile>&& children) {
        if (this->_children.size() > 0) {
            throw std::runtime_error("Children already created.");
        }
        this->_children = std::move(children);
    }

    void Tile::setContentUri(const std::optional<std::string>& value) {
        this->_contentUri = value;
    }

    void Tile::loadContent() {
        if (this->getState() != LoadState::Unloaded) {
            return;
        }

        if (!this->getContentUri().has_value()) {
            // TODO: should we let the renderer do some preparation even if there's no content?
            this->setState(LoadState::ContentLoaded);
            this->_pTileset->notifyTileDoneLoading(this);
            return;
        }

        this->setState(LoadState::ContentLoading);

        IAssetAccessor* pAssetAccessor = this->_pTileset->getExternals().pAssetAccessor;
        this->_pContentRequest = pAssetAccessor->requestAsset(*this->_contentUri);
        this->_pContentRequest->bind(std::bind(&Tile::contentResponseReceived, this, std::placeholders::_1));
    }

    bool Tile::unloadContent() {
        // Cannot unload while an async operation is in progress.
        // Also, don't unload tiles with external tileset content at all, because reloading
        // currently won't work correctly.
        if (
            this->getState() == Tile::LoadState::ContentLoading ||
            (this->getContent() != nullptr && this->getContent()->getType() == ExternalTilesetContent::TYPE)
        ) {
            return false;
        }

        const TilesetExternals& externals = this->_pTileset->getExternals();
        if (externals.pPrepareRendererResources) {
            if (this->getState() == LoadState::ContentLoaded) {
                externals.pPrepareRendererResources->free(*this, this->_pRendererResources, nullptr);
            } else {
                externals.pPrepareRendererResources->free(*this, nullptr, this->_pRendererResources);
            }
        }

        this->_pRendererResources = nullptr;
        this->_pContent.reset();
        this->setState(LoadState::Unloaded);

        return true;
    }

    void Tile::cancelLoadContent() {
        if (this->_pContentRequest) {
            this->_pContentRequest->cancel();
            this->_pContentRequest.release();

            if (this->getState() == LoadState::ContentLoading) {
                this->setState(LoadState::Unloaded);
            }
        }
    }

    void Tile::update(uint32_t previousFrameNumber, uint32_t currentFrameNumber) {
        if (this->getState() == LoadState::ContentLoaded) {
            const TilesetExternals& externals = this->_pTileset->getExternals();
            if (externals.pPrepareRendererResources) {
                this->_pRendererResources = externals.pPrepareRendererResources->prepareInMainThread(*this, this->getRendererResources());
            }

            TileContent* pContent = this->getContent();
            if (pContent) {
                pContent->finalizeLoad(*this);
            }

            this->setState(LoadState::Done);
        }
    }

    void Tile::setState(LoadState value) {
        this->_state.store(value, std::memory_order::memory_order_release);
    }

    void Tile::contentResponseReceived(IAssetRequest* pRequest) {
        if (this->getState() == LoadState::Destroying) {
            this->_pTileset->notifyTileDoneLoading(this);
            this->setState(LoadState::Failed);
            return;
        }

        if (this->getState() > LoadState::ContentLoading) {
            // This is a duplicate response, ignore it.
            return;
        }

        IAssetResponse* pResponse = pRequest->response();
        if (!pResponse) {
            // TODO: report the lack of response. Network error? Can this even happen?
            this->_pTileset->notifyTileDoneLoading(this);
            this->setState(LoadState::Failed);
            return;
        }

        if (pResponse->statusCode() < 200 || pResponse->statusCode() >= 300) {
            // TODO: report error response.
            this->_pTileset->notifyTileDoneLoading(this);
            this->setState(LoadState::Failed);
            return;
        }

        gsl::span<const uint8_t> data = pResponse->data();

        const TilesetExternals& externals = this->_pTileset->getExternals();

        externals.pTaskProcessor->startTask([data, this]() {
            if (this->getState() == LoadState::Destroying) {
                this->_pTileset->notifyTileDoneLoading(this);
                this->setState(LoadState::Failed);
                return;
            }

            std::unique_ptr<TileContent> pContent = TileContentFactory::createContent(*this, data, this->_pContentRequest->url());
            if (pContent) {
                this->_pContent = std::move(pContent);

                if (this->getState() == LoadState::Destroying) {
                    this->_pTileset->notifyTileDoneLoading(this);
                    this->setState(LoadState::Failed);
                    return;
                }

                const TilesetExternals& externals = this->_pTileset->getExternals();
                if (externals.pPrepareRendererResources) {
                    this->_pRendererResources = externals.pPrepareRendererResources->prepareInLoadThread(*this);
                }
                else {
                    this->_pRendererResources = nullptr;
                }
            }

            this->_pTileset->notifyTileDoneLoading(this);
            this->setState(LoadState::ContentLoaded);
        });
    }

}
