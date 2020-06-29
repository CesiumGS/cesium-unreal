#include "Cesium3DTiles/Tile.h"
#include "Cesium3DTiles/Tileset.h"
#include "Cesium3DTiles/IAssetAccessor.h"
#include "Cesium3DTiles/IAssetResponse.h"
#include "Cesium3DTiles/TileContentFactory.h"
#include <fstream>

namespace Cesium3DTiles {

    Tile::Tile(const Tileset& tileset, VectorReference<Tile> pParent) :
        _pTileset(&tileset),
        _pParent(pParent),
        _children(),
        _boundingVolume(BoundingBox(glm::dvec3(), glm::dmat4())),
        _viewerRequestVolume(),
        _geometricError(0.0),
        _refine(),
        _transform(),
        _contentUri(),
        _contentBoundingVolume(),
        _state(LoadState::Unloaded),
        _pContentRequest(nullptr),
        _pContent(nullptr),
        _pRendererResources(nullptr)
    {
    }

    Tile::~Tile() {
    }

    Tile::Tile(Tile&& rhs) noexcept :
        _pTileset(rhs._pTileset),
        _pParent(rhs._pParent),
        _children(rhs._children),
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
        _pRendererResources(rhs._pRendererResources)
    {
    }

    Tile& Tile::operator=(Tile&& rhs) noexcept {
        if (this != &rhs) {
            this->_pTileset = rhs._pTileset;
            this->_pParent = rhs._pParent;
            this->_children = rhs._children;
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
        }

        return *this;
    }

    void Tile::setChildren(const VectorRange<Tile>& children) {
        this->_children = children;
    }

    void Tile::setContentUri(const std::optional<std::string>& value)
    {
        this->_contentUri = value;
    }

    void Tile::loadContent()
    {
        if (this->getState() != LoadState::Unloaded) {
            return;
        }

        if (!this->getContentUri().has_value()) {
            return;
        }

        IAssetAccessor* pAssetAccessor = this->_pTileset->getExternals().pAssetAccessor;
        this->_pContentRequest = pAssetAccessor->requestAsset(*this->_contentUri);
        this->_pContentRequest->bind(std::bind(&Tile::contentResponseReceived, this, std::placeholders::_1));

        this->setState(LoadState::ContentLoading);
    }

    void Tile::setState(LoadState value)
    {
        this->_state.store(value, std::memory_order::memory_order_release);
    }

    void Tile::contentResponseReceived(IAssetRequest* pRequest) {
        IAssetResponse* pResponse = pRequest->response();
        if (!pResponse) {
            // TODO: report the lack of response. Network error? Can this even happen?
            return;
        }

        if (pResponse->statusCode() < 200 || pResponse->statusCode() >= 300) {
            // TODO: report error response.
            return;
        }

        gsl::span<const uint8_t> data = pResponse->data();

        const TilesetExternals& externals = this->_pTileset->getExternals();

        externals.pTaskProcessor->startTask([data, this]() {
            std::unique_ptr<TileContent> pContent = TileContentFactory::createContent(*this, data);

            if (!pContent) {
                // Try loading this content as an external tileset (JSON).
            }

            this->_pContent = std::move(pContent);
            this->setState(LoadState::ContentLoaded);

            const TilesetExternals& externals = this->_pTileset->getExternals();
            if (externals.pPrepareRendererResources) {
                this->setState(LoadState::RendererResourcesPreparing);
                externals.pPrepareRendererResources->prepare(*this);
            }
            else {
                this->setState(LoadState::RendererResourcesPrepared);
            }
            });
    }

    void Tile::finishPrepareRendererResources(void* pResource) {
        this->_pRendererResources = pResource;
        this->setState(LoadState::RendererResourcesPrepared);
    }

}
