#include "Cesium3DTiles/registerAllTileContentTypes.h"
#include "Cesium3DTiles/TileContentFactory.h"
#include "Cesium3DTiles/Batched3DModelContent.h"

namespace Cesium3DTiles {

    void registerAllTileContentTypes() {
        TileContentFactory::registerContentType(Batched3DModelContent::TYPE, [](const Tile& tile, const gsl::span<const uint8_t>& data) {
            return std::make_unique<Batched3DModelContent>(tile, data);
        });
    }

}
