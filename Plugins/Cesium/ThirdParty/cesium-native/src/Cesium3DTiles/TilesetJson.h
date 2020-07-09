#pragma once

#include <optional>
#include "Cesium3DTiles/BoundingVolume.h"
#include "Cesium3DTiles/Json.h"

namespace Cesium3DTiles {

    class TilesetJson {
    public:
        static std::optional<BoundingVolume> getBoundingVolumeProperty(const nlohmann::json& tileJson, const std::string& key);
        static std::optional<double> getScalarProperty(const nlohmann::json& tileJson, const std::string& key);
        static std::optional<glm::dmat4x4> getTransformProperty(const nlohmann::json& tileJson, const std::string& key);
    };

}