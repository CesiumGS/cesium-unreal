#pragma once

namespace Cesium3DTiles {
    
    class Tile;

    class TileContent {
    public:
        TileContent(const Tile& tile);
        virtual ~TileContent();

    private:
        // TODO: use VectorReference instead of a raw pointer
        const Tile* _pTile;
    };

}
