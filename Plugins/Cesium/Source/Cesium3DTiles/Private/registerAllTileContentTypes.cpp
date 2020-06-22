#include "registerAllTileContentTypes.h"
#include "TileContentFactory.h"
#include "Batched3DModelContent.h"

void registerAll3DTileContentTypes() {
    Cesium3DTileContentFactory::registerContentType("b3dm", [](const Cesium3DTile& tile, const gsl::span<const uint8_t>& data) {
        return std::make_unique<Batched3DModelContent>(tile, data);
    });
}
