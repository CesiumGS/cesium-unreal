#include "Cesium3DTiles/Tileset.h"
#include "Cesium3DTiles/IAssetAccessor.h"
#include "Cesium3DTiles/IAssetResponse.h"
#include "Uri.h"
#include "TilesetJson.h"

namespace Cesium3DTiles {

	Tileset::Tileset(
		const TilesetExternals& externals,
		const std::string& url,
		const TilesetOptions& options
	) :
		_externals(externals),
		_url(url),
		_ionAssetID(),
		_ionAccessToken(),
		_options(options),
		_pTilesetRequest(),
		_pRootTile(),
        _loadQueueHigh(),
		_loadQueueMedium(),
		_loadQueueLow(),
		_loadsInProgress(0),
		_loadedTiles()
	{
		this->_pTilesetRequest = this->_externals.pAssetAccessor->requestAsset(url);
		this->_pTilesetRequest->bind(std::bind(&Tileset::_tilesetJsonResponseReceived, this, std::placeholders::_1));
	}

	Tileset::Tileset(
		const TilesetExternals& externals,
		uint32_t ionAssetID,
		const std::string& ionAccessToken,
		const TilesetOptions& options
	) :
		_externals(externals),
		_url(),
		_ionAssetID(ionAssetID),
		_ionAccessToken(ionAccessToken),
		_options(options),
		_pTilesetRequest(),
		_pRootTile(),
        _loadQueueHigh(),
		_loadQueueMedium(),
		_loadQueueLow(),
		_loadsInProgress(0),
		_loadedTiles()
	{
		std::string url = "https://api.cesium.com/v1/assets/" + std::to_string(ionAssetID) + "/endpoint";
		if (ionAccessToken.size() > 0)
		{
			url += "?access_token=" + ionAccessToken;
		}

		this->_pTilesetRequest = this->_externals.pAssetAccessor->requestAsset(url);
		this->_pTilesetRequest->bind(std::bind(&Tileset::_ionResponseReceived, this, std::placeholders::_1));
	}

	Tileset::Tileset(
		const TilesetExternals& externals,
		const gsl::span<const uint8_t>& data,
		const std::string& url,
		const TilesetOptions& options
	) :
		_externals(externals),
		_url(url),
		_ionAssetID(),
		_ionAccessToken(),
		_options(options),
		_pTilesetRequest(),
		_pRootTile(),
        _loadQueueHigh(),
		_loadQueueMedium(),
		_loadQueueLow(),
		_loadsInProgress(0),
		_loadedTiles()
	{
		this->_loadTilesetJsonData(data, url);
	}

    const ViewUpdateResult& Tileset::updateView(const Camera& camera) {
		uint32_t previousFrameNumber = this->_previousFrameNumber; 
		uint32_t currentFrameNumber = previousFrameNumber + 1;

        ViewUpdateResult& result = this->_updateResult;
        // result.tilesLoading = 0;
        result.tilesToRenderThisFrame.clear();
        // result.newTilesToRenderThisFrame.clear();
        result.tilesToNoLongerRenderThisFrame.clear();

        Tile* pRootTile = this->getRootTile();
        if (!pRootTile) {
            return result;
        }

		this->_loadQueueHigh.clear();
		this->_loadQueueMedium.clear();
		this->_loadQueueLow.clear();

		this->_visitTileIfVisible(previousFrameNumber, currentFrameNumber, camera, false, *pRootTile, result);
		this->_unloadCachedTiles();
		this->_processLoadQueue();

        this->_previousFrameNumber = currentFrameNumber;

        return result;
    }

	void Tileset::notifyTileDoneLoading(Tile* pTile) {
		--this->_loadsInProgress;
	}

	void Tileset::loadTilesFromJson(Tile& rootTile, const nlohmann::json& tilesetJson, const std::string& baseUrl) {
		this->_createTile(rootTile, tilesetJson["root"], baseUrl);
	}

	void Tileset::_ionResponseReceived(IAssetRequest* pRequest) {
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
		this->_pTilesetRequest->bind(std::bind(&Tileset::_tilesetJsonResponseReceived, this, std::placeholders::_1));
	}

	void Tileset::_tilesetJsonResponseReceived(IAssetRequest* pRequest) {
		IAssetResponse* pResponse = pRequest->response();
		if (!pResponse) {
			// TODO: report the lack of response. Network error? Can this even happen?
			return;
		}

		if (pResponse->statusCode() < 200 || pResponse->statusCode() >= 300) {
			// TODO: report error response.
			return;
		}

		this->_loadTilesetJsonData(pResponse->data(), pRequest->url());

		pRequest = nullptr;
		pResponse = nullptr;
		this->_pTilesetRequest.reset();
	}

	void Tileset::_loadTilesetJsonData(const gsl::span<const uint8_t>& data, const std::string& baseUrl) {
		using nlohmann::json;
		json tileset = json::parse(data.begin(), data.end());

		json& rootJson = tileset["root"];

		this->_pRootTile = std::make_unique<Tile>();
		this->_pRootTile->setTileset(this);

		this->_createTile(*this->_pRootTile, rootJson, baseUrl);
	}

	void Tileset::_createTile(Tile& tile, const nlohmann::json& tileJson, const std::string& baseUrl) {
		using nlohmann::json;

		if (!tileJson.is_object())
		{
			return;
		}

		tile.setTileset(this);
		Tile* pParent = tile.getParent();

		std::optional<glm::dmat4x4> tileTransform = TilesetJson::getTransformProperty(tileJson, "transform");
		glm::dmat4x4 transform = tileTransform.value_or(glm::dmat4x4(1.0));

		if (tileTransform && pParent) {
			transform = pParent->getTransform() * transform;
		} else if (pParent) {
			transform = pParent->getTransform();
		}

		tile.setTransform(transform);

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
				tile.setContentUri(fullUri);
			} else {
				tile.finishPrepareRendererResources();
			}

			std::optional<BoundingVolume> contentBoundingVolume = TilesetJson::getBoundingVolumeProperty(*contentIt, "boundingVolume");
			if (contentBoundingVolume) {
				tile.setContentBoundingVolume(transformBoundingVolume(transform, contentBoundingVolume.value()));
			}
		}

		std::optional<BoundingVolume> boundingVolume = TilesetJson::getBoundingVolumeProperty(tileJson, "boundingVolume");
		if (!boundingVolume) {
			// TODO: report missing required property
			return;
		}

		std::optional<double> geometricError = TilesetJson::getScalarProperty(tileJson, "geometricError");
		if (!geometricError) {
			// TODO: report missing required property
			return;
		}

		tile.setBoundingVolume(transformBoundingVolume(transform, boundingVolume.value()));
		//tile->setBoundingVolume(boundingVolume.value());
		tile.setGeometricError(geometricError.value());

		std::optional<BoundingVolume> viewerRequestVolume = TilesetJson::getBoundingVolumeProperty(tileJson, "viewerRequestVolume");
		if (viewerRequestVolume) {
			tile.setViewerRequestVolume(transformBoundingVolume(transform, viewerRequestVolume.value()));
		}
		
		json::const_iterator refineIt = tileJson.find("refine");
		if (refineIt != tileJson.end()) {
			const std::string& refine = *refineIt;
			if (refine == "REPLACE") {
				tile.setRefine(Tile::Refine::Replace);
			} else if (refine == "ADD") {
				tile.setRefine(Tile::Refine::Add);
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

			tile.createChildTiles(childrenJson.size());
			gsl::span<Tile> childTiles = tile.getChildren();

			for (size_t i = 0; i < childrenJson.size(); ++i) {
				const json& childJson = childrenJson[i];
				Tile& child = childTiles[i];
				child.setParent(&tile);
				this->_createTile(child, childJson, baseUrl);
			}
		}
	}

    static void markTileNonRendered(TileSelectionState::Result lastResult, Tile& tile, ViewUpdateResult& result) {
        if (lastResult == TileSelectionState::Result::Rendered) {
            result.tilesToNoLongerRenderThisFrame.push_back(&tile);
        }
    }

    static void markTileNonRendered(uint32_t lastFrameNumber, Tile& tile, ViewUpdateResult& result) {
        TileSelectionState::Result lastResult = tile.getLastSelectionState().getResult(lastFrameNumber);
        markTileNonRendered(lastResult, tile, result);
    }

    static void markChildrenNonRendered(int32_t lastFrameNumber, TileSelectionState::Result lastResult, Tile& tile, ViewUpdateResult& result) {
        if (lastResult == TileSelectionState::Result::Refined) {
            for (Tile& child : tile.getChildren()) {
                TileSelectionState::Result childLastResult = child.getLastSelectionState().getResult(lastFrameNumber);
                markTileNonRendered(childLastResult, child, result);
                markChildrenNonRendered(lastFrameNumber, childLastResult, child, result);
            }
        }
    }

    static void markChildrenNonRendered(uint32_t lastFrameNumber, Tile& tile, ViewUpdateResult& result) {
        TileSelectionState::Result lastResult = tile.getLastSelectionState().getResult(lastFrameNumber);
        markChildrenNonRendered(lastFrameNumber, lastResult, tile, result);
    }

    static void markTileAndChildrenNonRendered(int32_t lastFrameNumber, Tile& tile, ViewUpdateResult& result) {
        TileSelectionState::Result lastResult = tile.getLastSelectionState().getResult(lastFrameNumber);
        markTileNonRendered(lastResult, tile, result);
        markChildrenNonRendered(lastFrameNumber, lastResult, tile, result);
    }

	// Visits a tile for possible rendering. When we call this function with a tile:
	//   * It is not yet known whether the tile is visible.
	//   * Its parent tile does _not_ meet the SSE (unless ancestorMeetsSse=true, see comments below).
	//   * The tile may or may not be renderable.
	//   * The tile has not yet been added to a load queue.
	Tileset::TraversalDetails Tileset::_visitTileIfVisible(
		uint32_t lastFrameNumber,
		uint32_t currentFrameNumber,
		const Camera& camera,
		bool ancestorMeetsSse,
		Tile& tile,
		ViewUpdateResult& result
	) {
		this->_markTileVisited(tile);

        const BoundingVolume& boundingVolume = tile.getBoundingVolume();
        if (!camera.isBoundingVolumeVisible(boundingVolume)) {
            markTileAndChildrenNonRendered(lastFrameNumber, tile, result);
			tile.setLastSelectionState(TileSelectionState(currentFrameNumber, TileSelectionState::Result::Culled));

			// Preload this culled sibling if requested.
			if (this->_options.preloadSiblings) {
				this->_loadQueueLow.push_back(&tile);
			}

            return TraversalDetails();
        }

		return this->_visitTile(lastFrameNumber, currentFrameNumber, camera, ancestorMeetsSse, tile, result);
	}

	// Visits a tile for possible rendering. When we call this function with a tile:
	//   * The tile has previously been determined to be visible.
	//   * Its parent tile does _not_ meet the SSE (unless ancestorMeetsSse=true, see comments below).
	//   * The tile may or may not be renderable.
	//   * The tile has not yet been added to a load queue.
    Tileset::TraversalDetails Tileset::_visitTile(
		uint32_t lastFrameNumber,
		uint32_t currentFrameNumber,
		const Camera& camera,
		bool ancestorMeetsSse,
		Tile& tile,
		ViewUpdateResult& result
	) {
		TileSelectionState lastFrameSelectionState = tile.getLastSelectionState();

		// If this is a leaf tile, just render it (it's already been deemed visible).
        gsl::span<Tile> children = tile.getChildren();
        if (children.size() == 0) {
            // Render this leaf tile.
			tile.setLastSelectionState(TileSelectionState(currentFrameNumber, TileSelectionState::Result::Rendered));
            result.tilesToRenderThisFrame.push_back(&tile);
			this->_loadQueueMedium.push_back(&tile);

			TraversalDetails result;
			result.allAreRenderable = tile.isRenderable();
			result.anyWereRenderedLastFrame = lastFrameSelectionState.getResult(lastFrameNumber) == TileSelectionState::Result::Rendered;
			result.notYetRenderableCount = result.allAreRenderable ? 0 : 1;
			return result;
        }

        const BoundingVolume& boundingVolume = tile.getBoundingVolume();
        double distanceSquared = camera.computeDistanceSquaredToBoundingVolume(boundingVolume);
        double distance = sqrt(distanceSquared);

        // Does this tile meet the screen-space error?
        double sse = camera.computeScreenSpaceError(tile.getGeometricError(), distance);
		bool meetsSse = sse < this->_options.maximumScreenSpaceError;

		// If we're forbidding holes, don't refine if any children are still loading.
		bool waitingForChildren = false;
		if (this->_options.forbidHoles) {
			for (Tile& child : children) {
				if (!child.isRenderable()) {
					waitingForChildren = true;
					this->_loadQueueMedium.push_back(&child);
				}
			}
		}

		if (meetsSse || ancestorMeetsSse || waitingForChildren) {
			// This tile (or an ancestor) is the one we want to render this frame, but we'll do different things depending
			// on the state of this tile and on what we did _last_ frame.

			// We can render it if _any_ of the following are true:
			// 1. We rendered it (or kicked it) last frame.
			// 2. This tile was culled last frame, or it wasn't even visited because an ancestor was culled.
			// 3. The tile is done loading and ready to render.
			//
			// Note that even if we decide to render a tile here, it may later get "kicked" in favor of an ancestor.
			TileSelectionState::Result originalResult = lastFrameSelectionState.getOriginalResult(lastFrameNumber);
			bool oneRenderedLastFrame = originalResult == TileSelectionState::Result::Rendered;
			bool twoCulledOrNotVisited = originalResult == TileSelectionState::Result::Culled || originalResult == TileSelectionState::Result::None;
			bool threeCompletelyLoaded = tile.isRenderable();

			bool renderThisTile = oneRenderedLastFrame || twoCulledOrNotVisited || threeCompletelyLoaded;
			if (renderThisTile) {
				// Only load this tile if it (not just an ancestor) meets the SSE.
				if (meetsSse) {
					this->_loadQueueMedium.push_back(&tile);
				}

				markChildrenNonRendered(lastFrameNumber, tile, result);
				tile.setLastSelectionState(TileSelectionState(currentFrameNumber, TileSelectionState::Result::Rendered));
				result.tilesToRenderThisFrame.push_back(&tile);

				TraversalDetails result;
				result.allAreRenderable = tile.isRenderable();
				result.anyWereRenderedLastFrame = lastFrameSelectionState.getResult(lastFrameNumber) == TileSelectionState::Result::Rendered;
				result.notYetRenderableCount = result.allAreRenderable ? 0 : 1;

				return result;
			}

			// Otherwise, we can't render this tile (or blank space where it would be) because doing so would cause detail to disappear
			// that was visible last frame. Instead, keep rendering any still-visible descendants that were rendered
			// last frame and render nothing for newly-visible descendants. E.g. if we were rendering level 15 last
			// frame but this frame we want level 14 and the closest renderable level <= 14 is 0, rendering level
			// zero would be pretty jarring so instead we keep rendering level 15 even though its SSE is better
			// than required. So fall through to continue traversal...
			ancestorMeetsSse = true;

			// Load this blocker tile with high priority, but only if this tile (not just an ancestor) meets the SSE.
			if (meetsSse) {
				this->_loadQueueHigh.push_back(&tile);
			}
		}

		// Refine!

		size_t firstRenderedDescendantIndex = result.tilesToRenderThisFrame.size();
		size_t loadIndexLow = this->_loadQueueLow.size();
		size_t loadIndexMedium = this->_loadQueueMedium.size();
		size_t loadIndexHigh = this->_loadQueueHigh.size();

		TraversalDetails traversalDetails = this->_visitVisibleChildrenNearToFar(lastFrameNumber, currentFrameNumber, camera, ancestorMeetsSse, tile, result);

		if (firstRenderedDescendantIndex == result.tilesToRenderThisFrame.size()) {
			// No descendant tiles were added to the render list by the function above, meaning they were all
			// culled even though this tile was deemed visible. That's pretty common.
			// Nothing else to do except mark this tile refined and return.
			markTileNonRendered(lastFrameNumber, tile, result);
			tile.setLastSelectionState(TileSelectionState(currentFrameNumber, TileSelectionState::Result::Refined));
			return TraversalDetails();
		}

		bool queuedForLoad = false;

		// At least one descendant tile was added to the render list.
		// The traversalDetails tell us what happened while visiting the children.
		if (!traversalDetails.allAreRenderable && !traversalDetails.anyWereRenderedLastFrame) {
			// Some of our descendants aren't ready to render yet, and none were rendered last frame,
			// so kick them all out of the render list and render this tile instead. Continue to load them though!

			std::vector<Tile*>& renderList = result.tilesToRenderThisFrame;
	        
			// Mark the rendered descendants and their ancestors - up to this tile - as kicked.
			for (size_t i = firstRenderedDescendantIndex; i < renderList.size(); ++i) {
				Tile* pWorkTile = renderList[i];
				while (
					pWorkTile != nullptr &&
					!pWorkTile->getLastSelectionState().wasKicked(currentFrameNumber) &&
					pWorkTile != &tile
				) {
					pWorkTile->getLastSelectionState().kick();
					pWorkTile = pWorkTile->getParent();
				}
			}

			// Remove all descendants from the render list and add this tile.
			renderList.erase(renderList.begin() + firstRenderedDescendantIndex, renderList.end());
			renderList.push_back(&tile);
			tile.setLastSelectionState(TileSelectionState(currentFrameNumber, TileSelectionState::Result::Rendered));

			// If we're waiting on heaps of descendants, the above will take too long. So in that case,
			// load this tile INSTEAD of loading any of the descendants, and tell the up-level we're only waiting
			// on this tile. Keep doing this until we actually manage to render this tile.
			bool wasRenderedLastFrame = lastFrameSelectionState.getResult(lastFrameNumber) == TileSelectionState::Result::Rendered;
			bool wasReallyRenderedLastFrame = wasRenderedLastFrame && tile.isRenderable();

			if (!wasReallyRenderedLastFrame && traversalDetails.notYetRenderableCount > this->_options.loadingDescendantLimit) {
				// Remove all descendants from the load queues.
				this->_loadQueueLow.erase(this->_loadQueueLow.begin() + loadIndexLow, this->_loadQueueLow.end());
				this->_loadQueueMedium.erase(this->_loadQueueMedium.begin() + loadIndexMedium, this->_loadQueueMedium.end());
				this->_loadQueueHigh.erase(this->_loadQueueHigh.begin() + loadIndexHigh, this->_loadQueueHigh.end());

				this->_loadQueueMedium.push_back(&tile);
				traversalDetails.notYetRenderableCount = tile.isRenderable() ? 0 : 1;
				queuedForLoad = true;
			}

			traversalDetails.allAreRenderable = tile.isRenderable();
			traversalDetails.anyWereRenderedLastFrame = wasRenderedLastFrame;
		} else {
			markTileNonRendered(lastFrameNumber, tile, result);
			tile.setLastSelectionState(TileSelectionState(currentFrameNumber, TileSelectionState::Result::Refined));
		}

		if (this->_options.preloadAncestors && !queuedForLoad) {
			this->_loadQueueLow.push_back(&tile);
		}

		return traversalDetails;
    }

	Tileset::TraversalDetails Tileset::_visitVisibleChildrenNearToFar(
		uint32_t lastFrameNumber,
		uint32_t currentFrameNumber,
		const Camera& camera,
		bool ancestorMeetsSse,
		Tile& tile,
		ViewUpdateResult& result
	) {
		TraversalDetails traversalDetails;

		// TODO: actually visit near-to-far, rather than in order of occurrence.
		gsl::span<Tile> children = tile.getChildren();
		for (Tile& child : children) {
			TraversalDetails childTraversal = this->_visitTileIfVisible(
				lastFrameNumber,
				currentFrameNumber,
				camera,
				ancestorMeetsSse,
				child,
				result
			);

			traversalDetails.allAreRenderable &= childTraversal.allAreRenderable;
			traversalDetails.anyWereRenderedLastFrame |= childTraversal.anyWereRenderedLastFrame;
			traversalDetails.notYetRenderableCount += childTraversal.notYetRenderableCount;
		}

		return traversalDetails;
	}

	static void processQueue(const std::vector<Tile*>& queue, std::atomic<uint32_t>& loadsInProgress, uint32_t maximumLoadsInProgress) {
		if (loadsInProgress >= maximumLoadsInProgress) {
			return;
		}

		for (Tile* pTile : queue) {
			if (pTile->getState() == Tile::LoadState::Unloaded) {
				++loadsInProgress;
				pTile->loadContent();

				if (loadsInProgress >= maximumLoadsInProgress) {
					break;
				}
			}
		}
	}

	void Tileset::_processLoadQueue() {
		processQueue(this->_loadQueueHigh, this->_loadsInProgress, this->_options.maximumSimultaneousTileLoads);
		processQueue(this->_loadQueueMedium, this->_loadsInProgress, this->_options.maximumSimultaneousTileLoads);
		processQueue(this->_loadQueueLow, this->_loadsInProgress, this->_options.maximumSimultaneousTileLoads);
	}

	void Tileset::_unloadCachedTiles() {
		Tile* pTile = this->_loadedTiles.head();

		while (this->_loadedTiles.size() > this->_options.maximumCachedTiles) {
			if (pTile == nullptr || pTile == this->_pRootTile.get()) {
				// We've either removed all tiles or the next tile is the root.
				// The root tile marks the beginning of the tiles that were used
				// for rendering last frame.
				break;
			}

			// Cannot unload while an async operation is in progress.
			if (
				pTile->getState() != Tile::LoadState::ContentLoading &&
				pTile->getState() != Tile::LoadState::RendererResourcesPreparing
			) {
				pTile->unloadContent();
			}

			Tile* pNext = this->_loadedTiles.next(*pTile);
			this->_loadedTiles.remove(*pTile);
			pTile = pNext;
		}
	}

	void Tileset::_markTileVisited(Tile& tile) {
		this->_loadedTiles.insertAtTail(tile);
	}

}
