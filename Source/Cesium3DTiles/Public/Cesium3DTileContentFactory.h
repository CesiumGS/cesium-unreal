#pragma once

#include <memory>
#include <functional>
#include <optional>
#include <gsl/span>

class Cesium3DTile;
class Cesium3DTileContent;

class Cesium3DTileContentFactory {
public:
    Cesium3DTileContentFactory() = delete;

    typedef std::unique_ptr<Cesium3DTileContent> FactoryFunctionSignature(const Cesium3DTile& tile, const gsl::span<const uint8_t>& data);
    typedef std::function<FactoryFunctionSignature> FactoryFunction;

    static void registerContentType(const std::string& magic, FactoryFunction factoryFunction);
    static std::unique_ptr<Cesium3DTileContent> createContent(const Cesium3DTile& tile, const gsl::span<const uint8_t>& data);

private:
    static std::optional<std::string> getMagic(const gsl::span<const uint8_t>& data);

    static std::unordered_map<std::string, FactoryFunction> _factoryFunctions;
};
