#pragma once

#include <gsl/span>
#include <tiny_gltf.h>
#include "Cesium3DTiles/Library.h"
#include "Cesium3DTiles/TileContent.h"

namespace Cesium3DTiles {

    class CESIUM3DTILES_API Batched3DModelContent : public TileContent {
    public:
        static std::string TYPE;

        Batched3DModelContent(const Tile& tile, const gsl::span<const uint8_t>& data, const std::string& url);

        const tinygltf::Model& gltf() const { return this->_gltf; }

        virtual const std::string& getType() const { return TYPE; }
        virtual void finalizeLoad(Tile& /*tile*/) {}

    private:
        tinygltf::Model _gltf;
    };

}
