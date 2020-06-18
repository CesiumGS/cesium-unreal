#include "Cesium3DTileContentFactory.h"
#include "Cesium3DTileContent.h"

void Cesium3DTileContentFactory::registerContentType(const std::string& magic, Cesium3DTileContentFactory::FactoryFunction factoryFunction) {
    Cesium3DTileContentFactory::_factoryFunctions[magic] = factoryFunction;
}

std::unique_ptr<Cesium3DTileContent> Cesium3DTileContentFactory::createContent(const Cesium3DTile& tile, const gsl::span<const uint8_t>& data) {
    std::string magic = Cesium3DTileContentFactory::getMagic(data).value_or("json");
    
    auto it = Cesium3DTileContentFactory::_factoryFunctions.find(magic);
    if (it == Cesium3DTileContentFactory::_factoryFunctions.end()) {
        it = Cesium3DTileContentFactory::_factoryFunctions.find("json");
    }

    if (it == Cesium3DTileContentFactory::_factoryFunctions.end()) {
        // No content type registered for this magic.
        return std::unique_ptr<Cesium3DTileContent>();
    }

    return it->second(tile, data);
}

std::optional<std::string> Cesium3DTileContentFactory::getMagic(const gsl::span<const uint8_t>& data) {
    if (data.size() >= 4) {
        gsl::span<const uint8_t> magicData = data.subspan(0, 4);
        return std::string(magicData.begin(), magicData.end());
    }

    return std::optional<std::string>();
}

std::unordered_map<std::string, Cesium3DTileContentFactory::FactoryFunction> Cesium3DTileContentFactory::_factoryFunctions;
