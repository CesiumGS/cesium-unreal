#pragma once

#include <string>
#include "Cesium3DTiles/Library.h"

namespace Cesium3DTiles {
    
    class Tile;

    class CESIUM3DTILES_API TileContent {
    public:
        TileContent(const Tile& tile);
        virtual ~TileContent();

        virtual const std::string& getType() const = 0;
        
        /**
         * Gives this content a chance to modify its tile. This is the final step of
         * the tile load process, after which the tile state moves from the
         * \ref LoadState::RendererResourcesPrepared state to the
         * \ref LoadState::Done state.
         */
        virtual void finalizeLoad(Tile& tile) = 0;

    private:
        // TODO: use VectorReference instead of a raw pointer
        const Tile* _pTile;
    };

}
