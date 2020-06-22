#include "TilesetView.h"
#include "Tileset.h"
#include "ViewUpdateResult.h"

namespace Cesium3DTiles {

    static void visitTile(Tile& tile, ViewUpdateResult& result);

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

        Tile* pRootTile = this->_tileset.rootTile();
        if (!pRootTile) {
            return result;
        }

        visitTile(*pRootTile, result);

        return result;
    }

    static void visitTile(Tile& tile, ViewUpdateResult& result) {
        // Render all the leaf tiles.

        if (tile.getChildren().size() > 0) {
            for (Tile& child : tile.getChildren()) {
                visitTile(child, result);
            }
        }
        else {
            tile.loadContent();
            if (tile.getState() == Tile::LoadState::RendererResourcesPrepared) {
                result.tilesToRenderThisFrame.push_back(&tile);
            }
        }
    }

}
