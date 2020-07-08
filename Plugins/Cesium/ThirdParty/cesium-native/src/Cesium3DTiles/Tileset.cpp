#include "Cesium3DTiles/Tileset.h"
#include "Cesium3DTiles/IAssetAccessor.h"
#include "Cesium3DTiles/IAssetResponse.h"
#include "Uri.h"
#pragma warning(push)
#pragma warning(disable: 4946)
#include "json.hpp"
#pragma warning(pop)

namespace Cesium3DTiles {

	Tileset::Tileset(const TilesetExternals& externals, const std::string& url) :
		_externals(externals),
		_url(url),
		_ionAssetID(),
		_ionAccessToken(),
		_pTilesetRequest(),
		_tiles(),
		_pRootTile(),
        _loadQueue()
	{
		this->_pTilesetRequest = this->_externals.pAssetAccessor->requestAsset(url);
		this->_pTilesetRequest->bind(std::bind(&Tileset::tilesetJsonResponseReceived, this, std::placeholders::_1));
	}

	Tileset::Tileset(const TilesetExternals& externals, uint32_t ionAssetID, const std::string& ionAccessToken) :
		_externals(externals),
		_url(),
		_ionAssetID(ionAssetID),
		_ionAccessToken(ionAccessToken),
		_pTilesetRequest(),
		_tiles(),
		_pRootTile(),
        _loadQueue()
	{
		std::string url = "https://api.cesium.com/v1/assets/" + std::to_string(ionAssetID) + "/endpoint";
		if (ionAccessToken.size() > 0)
		{
			url += "?access_token=" + ionAccessToken;
		}

		this->_pTilesetRequest = this->_externals.pAssetAccessor->requestAsset(url);
		this->_pTilesetRequest->bind(std::bind(&Tileset::ionResponseReceived, this, std::placeholders::_1));
	}

    const ViewUpdateResult& Tileset::updateView(const Camera& camera) {
		if (this->_currentFrameNumber.has_value()) {
			throw std::runtime_error("Cannot update the view while an update is already in progress.");
		}

		uint32_t previousFrameNumber = this->_previousFrameNumber; 
		uint32_t currentFrameNumber = previousFrameNumber + 1;
		this->_currentFrameNumber = currentFrameNumber;

        ViewUpdateResult& result = this->_updateResult;
        result.tilesLoading = 0;
        result.tilesToRenderThisFrame.clear();
        result.newTilesToRenderThisFrame.clear();
        result.tilesToNoLongerRenderThisFrame.clear();

        Tile* pRootTile = this->getRootTile();
        if (!pRootTile) {
            return result;
        }

		this->_loadQueue.clear();

        if (pRootTile->getState() == Tile::LoadState::RendererResourcesPrepared) {
	        this->_visitTile(previousFrameNumber, currentFrameNumber, camera, 16.0, *pRootTile, result);
		} else {
			// Root tile is not loaded yet, so do that first.
            pRootTile->loadContent();
        }

		this->_processLoadQueue();

        this->_previousFrameNumber = currentFrameNumber;
		this->_currentFrameNumber.reset();

        return result;
    }

	void Tileset::ionResponseReceived(IAssetRequest* pRequest) {
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

		using nlohmann::json;
		json ionResponse = json::parse(data.begin(), data.end());

		std::string url = ionResponse.value<std::string>("url", "");
		std::string accessToken = ionResponse.value<std::string>("accessToken", "");
		std::string urlWithToken = Uri::addQuery(url, "access_token", accessToken);

		// When we assign _pTilesetRequest, the previous request and response
		// that we're currently handling may immediately be deleted.
		pRequest = nullptr;
		pResponse = nullptr;
		this->_pTilesetRequest = this->_externals.pAssetAccessor->requestAsset(urlWithToken);
		this->_pTilesetRequest->bind(std::bind(&Tileset::tilesetJsonResponseReceived, this, std::placeholders::_1));
	}

	void Tileset::tilesetJsonResponseReceived(IAssetRequest* pRequest) {
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

		using nlohmann::json;
		json tileset = json::parse(data.begin(), data.end());

		std::string baseUrl = pRequest->url();

		pRequest = nullptr;
		pResponse = nullptr;
		this->_pTilesetRequest.reset();

		json& rootJson = tileset["root"];

		this->_tiles.emplace_back(*this);
		VectorReference<Tile> rootTile(this->_tiles, this->_tiles.size() - 1);

		this->createTile(rootTile, rootJson, baseUrl);
		this->_pRootTile = rootTile;
	}

	static std::optional<BoundingVolume> getBoundingVolumeProperty(const nlohmann::json& tileJson, const std::string& key) {
		using nlohmann::json;

		json::const_iterator bvIt = tileJson.find(key);
		if (bvIt == tileJson.end()) {
			return std::optional<BoundingVolume>();
		}

		json::const_iterator boxIt = bvIt->find("box");
		if (boxIt != bvIt->end() && boxIt->is_array() && boxIt->size() >= 12) {
			const json& a = *boxIt;
			return BoundingBox(
				glm::dvec3(a[0], a[1], a[2]),
				glm::dmat3(a[3], a[4], a[5], a[6], a[7], a[8], a[9], a[10], a[11])
			);
		}

		json::const_iterator regionIt = bvIt->find("region");
		if (regionIt != bvIt->end() && regionIt->is_array() && boxIt->size() >= 6) {
			const json& a = *boxIt;
			return BoundingRegion(a[0], a[1], a[2], a[3], a[4], a[5]);
		}

		json::const_iterator sphereIt = bvIt->find("sphere");
		if (sphereIt != bvIt->end() && sphereIt->is_array() && boxIt->size() >= 4) {
			const json& a = *boxIt;
			return BoundingSphere(glm::dvec3(a[0], a[1], a[2]), a[3]);
		}

		return std::optional<BoundingVolume>();
	}

	static std::optional<double> getScalarProperty(const nlohmann::json& tileJson, const std::string& key) {
		using nlohmann::json;

		json::const_iterator it = tileJson.find(key);
		if (it == tileJson.end() || !it->is_number()) {
			return std::optional<double>();
		}

		return it->get<double>();
	}

	static std::optional<glm::dmat4x4> getTransformProperty(const nlohmann::json& tileJson, const std::string& key) {
		using nlohmann::json;

		json::const_iterator it = tileJson.find(key);
		if (it == tileJson.end() || !it->is_array() || it->size() < 16) {
			return std::optional<glm::dmat4x4>();
		}

		const json& a = *it;
		return glm::dmat4(
			glm::dvec4(a[0], a[1], a[2], a[3]),
			glm::dvec4(a[4], a[5], a[6], a[7]),
			glm::dvec4(a[8], a[9], a[10], a[11]),
			glm::dvec4(a[12], a[13], a[14], a[15])
		);
	}

	void Tileset::createTile(VectorReference<Tile>& tile, const nlohmann::json& tileJson, const std::string& baseUrl) {
		using nlohmann::json;

		if (!tileJson.is_object())
		{
			return;
		}

		Tile* pParent = tile->getParent();

		std::optional<glm::dmat4x4> tileTransform = getTransformProperty(tileJson, "transform");
		glm::dmat4x4 transform = tileTransform.value_or(glm::dmat4x4(1.0));

		if (tileTransform && pParent) {
			transform = pParent->getTransform() * transform;
		} else if (pParent) {
			transform = pParent->getTransform();
		}

		tile->setTransform(transform);

		json::const_iterator contentIt = tileJson.find("content");
		json::const_iterator childrenIt = tileJson.find("children");

		if (contentIt != tileJson.end())
		{
			json::const_iterator uriIt = contentIt->find("uri");
			if (uriIt == contentIt->end()) {
				uriIt = contentIt->find("url");
			}

			if (uriIt != contentIt->end()) {
				const std::string& uri = *uriIt;
				const std::string fullUri = Uri::resolve(baseUrl, uri, true);
				tile->setContentUri(fullUri);
			} else {
				tile->finishPrepareRendererResources();
			}

			std::optional<BoundingVolume> contentBoundingVolume = getBoundingVolumeProperty(*contentIt, "boundingVolume");
			if (contentBoundingVolume) {
				tile->setContentBoundingVolume(transformBoundingVolume(transform, contentBoundingVolume.value()));
			}
		} else {
			tile->finishPrepareRendererResources();
		}

		std::optional<BoundingVolume> boundingVolume = getBoundingVolumeProperty(tileJson, "boundingVolume");
		if (!boundingVolume) {
			// TODO: report missing required property
			return;
		}

		std::optional<double> geometricError = getScalarProperty(tileJson, "geometricError");
		if (!geometricError) {
			// TODO: report missing required property
			return;
		}

		tile->setBoundingVolume(transformBoundingVolume(transform, boundingVolume.value()));
		//tile->setBoundingVolume(boundingVolume.value());
		tile->setGeometricError(geometricError.value());

		std::optional<BoundingVolume> viewerRequestVolume = getBoundingVolumeProperty(tileJson, "viewerRequestVolume");
		if (viewerRequestVolume) {
			tile->setViewerRequestVolume(transformBoundingVolume(transform, viewerRequestVolume.value()));
		}
		
		json::const_iterator refineIt = tileJson.find("refine");
		if (refineIt != tileJson.end()) {
			const std::string& refine = *refineIt;
			if (refine == "REPLACE") {
				tile->setRefine(Tile::Refine::Replace);
			} else if (refine == "ADD") {
				tile->setRefine(Tile::Refine::Add);
			} else {
				// TODO: report invalid value
			}
		}

		if (childrenIt != tileJson.end())
		{
			const json& childrenJson = *childrenIt;
			if (!childrenJson.is_array())
			{
				return;
			}

			// Allocate children contiguously and efficiently from a vector.
			size_t firstChild = this->_tiles.size();

			for (size_t i = 0; i < childrenJson.size(); ++i) {
				this->_tiles.emplace_back(*this, tile);
			}

			size_t afterLastChild = this->_tiles.size();

			for (size_t i = 0; i < childrenJson.size(); ++i) {
				const json& childJson = childrenJson[i];
				VectorReference<Tile> child(this->_tiles, firstChild + i);
				this->createTile(child, childJson, baseUrl);
			}

			VectorRange<Tile> childTiles(this->_tiles, firstChild, afterLastChild);
			tile->setChildren(childTiles);
		}
	}

    static TileSelectionState::Result getTileLastSelectionResult(uint32_t lastFrameNumber, Tile& tile) {
		return tile.getLastSelectionResult(lastFrameNumber);
    }

    static void markTileNonRendered(TileSelectionState::Result lastResult, Tile& tile, ViewUpdateResult& result) {
        if (lastResult == TileSelectionState::Result::Rendered) {
            result.tilesToNoLongerRenderThisFrame.push_back(&tile);
            //tile.cancelLoadContent();
        }
    }

    static void markTileNonRendered(uint32_t lastFrameNumber, Tile& tile, ViewUpdateResult& result) {
        TileSelectionState::Result lastResult = getTileLastSelectionResult(lastFrameNumber, tile);
        markTileNonRendered(lastResult, tile, result);
    }

    static void markChildrenNonRendered(int32_t lastFrameNumber, TileSelectionState::Result lastResult, Tile& tile, ViewUpdateResult& result) {
        if (lastResult == TileSelectionState::Result::Refined) {
            for (Tile& child : tile.getChildren()) {
                TileSelectionState::Result childLastResult = getTileLastSelectionResult(lastFrameNumber, child);
                markTileNonRendered(childLastResult, child, result);
                markChildrenNonRendered(lastFrameNumber, childLastResult, child, result);
            }
        }
    }

    static void markChildrenNonRendered(uint32_t lastFrameNumber, Tile& tile, ViewUpdateResult& result) {
        TileSelectionState::Result lastResult = getTileLastSelectionResult(lastFrameNumber, tile);
        markChildrenNonRendered(lastFrameNumber, lastResult, tile, result);
    }

    static void markTileAndChildrenNonRendered(int32_t lastFrameNumber, Tile& tile, ViewUpdateResult& result) {
        TileSelectionState::Result lastResult = getTileLastSelectionResult(lastFrameNumber, tile);
        markTileNonRendered(lastResult, tile, result);
        markChildrenNonRendered(lastFrameNumber, lastResult, tile, result);
    }

    void Tileset::_visitTile(uint32_t lastFrameNumber, uint32_t currentFrameNumber, const Camera& camera, double maximumScreenSpaceError, Tile& tile, ViewUpdateResult& result) {
        // Is this tile visible?
        const BoundingVolume& boundingVolume = tile.getBoundingVolume();
        if (!camera.isBoundingVolumeVisible(boundingVolume)) {
            markTileAndChildrenNonRendered(lastFrameNumber, tile, result);
			tile.setLastSelectionResult(currentFrameNumber, TileSelectionState::Result::Culled);
            return;
        }

        double distanceSquared = camera.computeDistanceSquaredToBoundingVolume(boundingVolume);
        double distance = sqrt(distanceSquared);

        VectorRange<Tile> children = tile.getChildren();
        if (children.size() == 0) {
            // Render this leaf tile.
			tile.setLastSelectionResult(currentFrameNumber, TileSelectionState::Result::Rendered);
            result.tilesToRenderThisFrame.push_back(&tile);
        }

        // Does this tile meet the screen-space error?
        double sse = camera.computeScreenSpaceError(tile.getGeometricError(), distance);

        if (sse <= maximumScreenSpaceError) {
            // Tile meets SSE requirements, render it.
            markChildrenNonRendered(lastFrameNumber, tile, result);
            tile.setLastSelectionResult(currentFrameNumber, TileSelectionState::Result::Rendered);
            result.tilesToRenderThisFrame.push_back(&tile);
            return;
        }

        bool allChildrenAreReady = true;
        for (Tile& child : children) {
			if (child.getState() != Tile::LoadState::RendererResourcesPrepared) {
				this->_loadQueue.push_back(&child);
				allChildrenAreReady = false;
			}
        }

        if (!allChildrenAreReady) {
            // Can't refine because all children aren't yet ready, so render this tile for now.
            markChildrenNonRendered(lastFrameNumber, tile, result);
            tile.setLastSelectionResult(currentFrameNumber, TileSelectionState::Result::Rendered);
            result.tilesToRenderThisFrame.push_back(&tile);
            return;
        }

        markTileNonRendered(lastFrameNumber, tile, result);
        tile.setLastSelectionResult(currentFrameNumber, TileSelectionState::Result::Refined);

        for (Tile& child : children) {
            this->_visitTile(lastFrameNumber, currentFrameNumber, camera, maximumScreenSpaceError, child, result);
        }
    }

	void Tileset::_processLoadQueue() {
		for (Tile* pTile : this->_loadQueue) {
			if (pTile->getState() == Tile::LoadState::Unloaded) {
				pTile->loadContent();
			}
		}
	}

}
