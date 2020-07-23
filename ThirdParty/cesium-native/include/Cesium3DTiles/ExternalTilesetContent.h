#pragma once

#include "Cesium3DTiles/TileContent.h"
#include "Cesium3DTiles/Tile.h"
#include <gsl/span>
#include <vector>

namespace Cesium3DTiles {

    class ExternalTilesetContent : public TileContent {
    public:
        static std::string TYPE;

        ExternalTilesetContent(const Tile& tile, const gsl::span<const uint8_t>& data, const std::string& url);

        virtual const std::string& getType() const { return ExternalTilesetContent::TYPE; }
        virtual void finalizeLoad(Tile& tile);

    private:
        std::vector<Tile> _externalRoot;
    };

}
