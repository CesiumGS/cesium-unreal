#pragma once

#include <vector>

namespace Cesium3DTiles {
    class Tile;

    class ViewUpdateResult {
    public:
        std::vector<Tile*> tilesToRenderThisFrame;

        // std::vector<Tile*> newTilesToRenderThisFrame;
        std::vector<Tile*> tilesToNoLongerRenderThisFrame;

        // uint32_t tilesLoading;
    };

}
