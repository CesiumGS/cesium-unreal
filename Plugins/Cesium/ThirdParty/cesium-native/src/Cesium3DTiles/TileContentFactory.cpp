#include "Cesium3DTiles/TileContentFactory.h"
#include "Cesium3DTiles/TileContent.h"

namespace Cesium3DTiles {

    void TileContentFactory::registerContentType(const std::string& magic, TileContentFactory::FactoryFunction factoryFunction) {
        TileContentFactory::_factoryFunctions[magic] = factoryFunction;
    }

    std::unique_ptr<TileContent> TileContentFactory::createContent(const Cesium3DTiles::Tile& tile, const gsl::span<const uint8_t>& data, const std::string& url) {
        std::string magic = TileContentFactory::getMagic(data).value_or("json");

        auto it = TileContentFactory::_factoryFunctions.find(magic);
        if (it == TileContentFactory::_factoryFunctions.end()) {
            it = TileContentFactory::_factoryFunctions.find("json");
        }

        if (it == TileContentFactory::_factoryFunctions.end()) {
            // No content type registered for this magic.
            return std::unique_ptr<TileContent>();
        }

        return it->second(tile, data, url);
    }

    std::optional<std::string> TileContentFactory::getMagic(const gsl::span<const uint8_t>& data) {
        if (data.size() >= 4) {
            gsl::span<const uint8_t> magicData = data.subspan(0, 4);
            return std::string(magicData.begin(), magicData.end());
        }

        return std::optional<std::string>();
    }

    std::unordered_map<std::string, TileContentFactory::FactoryFunction> TileContentFactory::_factoryFunctions;

}
