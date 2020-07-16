#include "Cesium3DTiles/ExternalTilesetContent.h"
#include "Cesium3DTiles/Json.h"
#include "Cesium3DTiles/Tile.h"
#include "Cesium3DTiles/Tileset.h"

namespace Cesium3DTiles {

    std::string ExternalTilesetContent::TYPE = "json";

    ExternalTilesetContent::ExternalTilesetContent(
        const Tile& tile,
        const gsl::span<const uint8_t>& data,
        const std::string& url
    ) :
        TileContent(tile),
        _externalRoot(1)
    {
        using nlohmann::json;

        json tilesetJson = json::parse(data.begin(), data.end());
        this->_externalRoot[0].setParent(const_cast<Tile*>(&tile));
        tile.getTileset()->loadTilesFromJson(this->_externalRoot[0], tilesetJson, url);
    }

    void ExternalTilesetContent::finalizeLoad(Tile& tile) {
        tile.createChildTiles(std::move(this->_externalRoot));
        tile.setGeometricError(999999999.0);
    }

}
