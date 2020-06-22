#pragma once

#include <optional>
#include <gsl/span>
#include <tiny_gltf.h>

namespace Cesium3DTiles {

    class CESIUM3DTILES_API Gltf {
    public:
        struct LoadResult {
        public:
            std::optional<tinygltf::Model> model;
            std::string warnings;
            std::string errors;
        };

        static LoadResult load(const gsl::span<const uint8_t>& data);
    };

}
