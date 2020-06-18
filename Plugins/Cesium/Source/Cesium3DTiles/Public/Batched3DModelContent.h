#pragma once

#include <gsl/span>
#include "Cesium3DTileContent.h"
#include "tiny_gltf.h"

class Batched3DModelContent : public Cesium3DTileContent {
public:
    Batched3DModelContent(const Cesium3DTile& tile, const gsl::span<const uint8_t>& data);

    const tinygltf::Model& gltf() const { return this->_gltf; }

private:
    tinygltf::Model _gltf;
};