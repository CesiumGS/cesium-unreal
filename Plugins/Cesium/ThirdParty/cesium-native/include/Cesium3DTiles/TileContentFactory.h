#pragma once

#include <memory>
#include <functional>
#include <optional>
#include <gsl/span>

namespace Cesium3DTiles {
    class Tile;
    class TileContent;

    class TileContentFactory {
    public:
        TileContentFactory() = delete;

        typedef std::unique_ptr<TileContent> FactoryFunctionSignature(const Tile& tile, const gsl::span<const uint8_t>& data);
        typedef std::function<FactoryFunctionSignature> FactoryFunction;

        static void registerContentType(const std::string& magic, FactoryFunction factoryFunction);
        static std::unique_ptr<TileContent> createContent(const Tile& tile, const gsl::span<const uint8_t>& data);

    private:
        static std::optional<std::string> getMagic(const gsl::span<const uint8_t>& data);

        static std::unordered_map<std::string, FactoryFunction> _factoryFunctions;
    };

}
