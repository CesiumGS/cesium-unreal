#include "Cesium3DTiles/registerAllTileContentTypes.h"
#include "Cesium3DTiles/TileContentFactory.h"
#include "Cesium3DTiles/Batched3DModelContent.h"
#include "Cesium3DTiles/ExternalTilesetContent.h"

namespace Cesium3DTiles {

    void registerAllTileContentTypes() {
        TileContentFactory::registerContentType(Batched3DModelContent::TYPE, [](const Tile& tile, const gsl::span<const uint8_t>& data, const std::string& url) {
            return std::make_unique<Batched3DModelContent>(tile, data, url);
        });

        TileContentFactory::registerContentType(ExternalTilesetContent::TYPE, [](const Tile& tile, const gsl::span<const uint8_t>& data, const std::string& url) {
            return std::make_unique<ExternalTilesetContent>(tile, data, url);
        });
    }

}
