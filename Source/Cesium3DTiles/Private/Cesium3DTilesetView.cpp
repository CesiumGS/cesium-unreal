#include "Cesium3DTilesetView.h"
#include "Cesium3DTileset.h"
#include "ViewUpdateResult.h"

static void visitTile(Cesium3DTile& tile, ViewUpdateResult& result);

Cesium3DTilesetView::Cesium3DTilesetView(Cesium3DTileset& tileset, const std::string& name) :
    _tileset(tileset),
    _name(name)
{

}

const ViewUpdateResult& Cesium3DTilesetView::update(const Camera& camera) {
    ViewUpdateResult& result = this->_updateResult;
    result.tilesLoading = 0;
    result.tilesToRenderThisFrame.clear();
    result.newTilesToRenderThisFrame.clear();
    result.tilesToNoLongerRenderThisFrame.clear();

    Cesium3DTile* pRootTile = this->_tileset.rootTile();
    if (!pRootTile) {
        return result;
    }

    visitTile(*pRootTile, result);

    return result;
}

static void visitTile(Cesium3DTile& tile, ViewUpdateResult& result) {
    // Render all the leaf tiles.

    if (tile.children().size() > 0) {
        for (Cesium3DTile& child : tile.children()) {
            visitTile(child, result);
        }
    } else {
        tile.loadContent();
        if (tile.isContentLoaded()) {
            result.tilesToRenderThisFrame.push_back(&tile);
        }
    }
}
