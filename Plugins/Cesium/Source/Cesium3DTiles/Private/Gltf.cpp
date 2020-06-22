#include "Gltf.h"

namespace Cesium3DTiles {
    Gltf::LoadResult Gltf::load(const gsl::span<const uint8_t>& data)
    {
        tinygltf::TinyGLTF loader;
        tinygltf::Model model;
        std::string errors;
        std::string warnings;

        bool loadSucceeded = loader.LoadBinaryFromMemory(&model, &errors, &warnings, data.data(), data.size());

        return LoadResult{
            model,
            warnings,
            errors
        };
    }
}
