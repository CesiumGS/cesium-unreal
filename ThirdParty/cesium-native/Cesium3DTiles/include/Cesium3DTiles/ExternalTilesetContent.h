#pragma once

#include <gsl/span>
#include <vector>
#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/TileContent.h"
#include "Cesium3DTiles/Tile.h"

namespace Cesium3DTiles {

    class CESIUM3DTILES_API ExternalTilesetContent : public TileContent {
    public:
        static std::string TYPE;

        ExternalTilesetContent(const Tile& tile, const gsl::span<const uint8_t>& data, const std::string& url);

        virtual const std::string& getType() const { return ExternalTilesetContent::TYPE; }
        virtual void finalizeLoad(Tile& tile);

    private:
        std::vector<Tile> _externalRoot;
    };

}
