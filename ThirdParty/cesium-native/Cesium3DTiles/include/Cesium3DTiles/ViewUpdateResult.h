#pragma once

#include <vector>
#include "Cesium3DTiles/Library.h"

namespace Cesium3DTiles {
    class Tile;

    class CESIUM3DTILES_API ViewUpdateResult {
    public:
        std::vector<Tile*> tilesToRenderThisFrame;

        // std::vector<Tile*> newTilesToRenderThisFrame;
        std::vector<Tile*> tilesToNoLongerRenderThisFrame;

        // uint32_t tilesLoading;
    };

}
