#include "registerAllTileContentTypes.h"
#include "TileContentFactory.h"
#include "Batched3DModelContent.h"

namespace Cesium3DTiles {

    void registerAllTileContentTypes() {
        TileContentFactory::registerContentType("b3dm", [](const Tile& tile, const gsl::span<const uint8_t>& data) {
            return std::make_unique<Batched3DModelContent>(tile, data);
        });
    }

}
