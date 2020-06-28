#include "Cesium3DTiles/TilesetView.h"
#include "Cesium3DTiles/Tileset.h"
#include "Cesium3DTiles/ViewUpdateResult.h"

namespace Cesium3DTiles {

    TilesetView::TilesetView(Cesium3DTiles::Tileset& tileset, const std::string& name) :
        _tileset(tileset),
        _name(name)
    {

    }

    const ViewUpdateResult& TilesetView::update(const Camera& camera) {
        ViewUpdateResult& result = this->_updateResult;
        result.tilesLoading = 0;
        result.tilesToRenderThisFrame.clear();
        result.newTilesToRenderThisFrame.clear();
        result.tilesToNoLongerRenderThisFrame.clear();

        Tile* pRootTile = this->_tileset.getRootTile();
        if (!pRootTile) {
            return result;
        }

        int32_t lastFrameNumber = this->_lastFrameNumber;

        this->_visitTile(lastFrameNumber, camera, 0.0, *pRootTile, result);

        this->_lastFrameNumber = lastFrameNumber + 1;

        return result;
    }

    void markChildrenNonRendered(std::unordered_map<Tile*, TileSelectionState>& lastSelectionResults, int32_t lastFrameNumber, Tile& tile, ViewUpdateResult& result, bool skipTile) {
        auto it = lastSelectionResults.find(&tile);
        if (it == lastSelectionResults.end()) {
            return;
        }

        TileSelectionState::Result selectionResult = it->second.getResult(lastFrameNumber);
        if (!skipTile && selectionResult == TileSelectionState::Result::Rendered) {
            result.tilesToNoLongerRenderThisFrame.push_back(&tile);
        } else if (selectionResult == TileSelectionState::Result::Refined) {
            for (Tile& child : tile.getChildren()) {
                markChildrenNonRendered(lastSelectionResults, lastFrameNumber, child, result, false);
            }
        }
    }


    void TilesetView::_visitTile(int32_t lastFrameNumber, const Camera& camera, double maximumScreenSpaceError, Tile& tile, ViewUpdateResult& result) {
        // Is this tile renderable?
        if (tile.getState() != Tile::LoadState::RendererResourcesPrepared) {
            tile.loadContent();
            return;
        }

        // Is this tile visible?
        const BoundingVolume& boundingVolume = tile.getBoundingVolume();
        if (!camera.isBoundingVolumeVisible(boundingVolume)) {
            markChildrenNonRendered(this->_lastSelectionResults, lastFrameNumber, tile, result, false);
            this->_lastSelectionResults[&tile] = TileSelectionState(lastFrameNumber + 1, TileSelectionState::Result::Culled);
            return;
        }

        double distance = camera.computeDistanceToBoundingVolume(boundingVolume);

        VectorRange<Tile> children = tile.getChildren();
        if (children.size() == 0) {
            // Render this leaf tile.
            markChildrenNonRendered(this->_lastSelectionResults, lastFrameNumber, tile, result, true);
            this->_lastSelectionResults[&tile] = TileSelectionState(lastFrameNumber + 1, TileSelectionState::Result::Rendered);
            result.tilesToRenderThisFrame.push_back(&tile);
        }

        // Does this tile meet the screen-space error?
        double sse = camera.computeScreenSpaceError(tile.getGeometricError(), distance);

        if (sse <= maximumScreenSpaceError) {
            // Tile meets SSE requirements, render it.
            markChildrenNonRendered(this->_lastSelectionResults, lastFrameNumber, tile, result, true);
            this->_lastSelectionResults[&tile] = TileSelectionState(lastFrameNumber + 1, TileSelectionState::Result::Rendered);
            result.tilesToRenderThisFrame.push_back(&tile);
            return;
        }

        bool allChildrenAreReady = true;
        for (Tile& child : children) {
            child.loadContent();
            allChildrenAreReady &= child.getState() == Tile::LoadState::RendererResourcesPrepared;
        }

        if (!allChildrenAreReady) {
            // Can't refine because all children aren't yet ready, so render this tile for now.
            markChildrenNonRendered(this->_lastSelectionResults, lastFrameNumber, tile, result, true);
            this->_lastSelectionResults[&tile] = TileSelectionState(lastFrameNumber + 1, TileSelectionState::Result::Rendered);
            result.tilesToRenderThisFrame.push_back(&tile);
            return;
        }

        markChildrenNonRendered(this->_lastSelectionResults, lastFrameNumber, tile, result, false);
        this->_lastSelectionResults[&tile] = TileSelectionState(lastFrameNumber + 1, TileSelectionState::Result::Refined);

        for (Tile& child : children) {
            child.loadContent();
            this->_visitTile(lastFrameNumber, camera, maximumScreenSpaceError, child, result);
        }
    }

}
