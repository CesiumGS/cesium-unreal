#pragma once

#include <string>

namespace Cesium3DTiles {
    
    class Tile;

    class TileContent {
    public:
        TileContent(const Tile& tile);
        virtual ~TileContent();

        virtual const std::string& getType() const = 0;

    private:
        // TODO: use VectorReference instead of a raw pointer
        const Tile* _pTile;
    };

}
