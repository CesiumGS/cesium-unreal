// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumGltfTextures.h"
#include "CesiumRuntime.h"
#include "CesiumTextureResource.h"
#include "CesiumTextureUtility.h"
#include "ExtensionImageAssetUnreal.h"
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltf/VertexAttributeSemantics.h>
#include <CesiumGltfReader/GltfReader.h>

using namespace CesiumAsync;
using namespace CesiumGltf;

namespace {

// Determines if a glTF primitive is usable for our purposes.
bool isValidPrimitive(const Model& gltf, const MeshPrimitive& primitive);

// Determines if an Accessor's componentType is valid for an index buffer.
bool isSupportedIndexComponentType(int32_t componentType);

// Determines if the given Primitive mode is one that we support.
bool isSupportedPrimitiveMode(int32_t primitiveMode);

// Determines if the given texture uses mipmaps.
bool doesTextureUseMipmaps(const Model& gltf, const Texture& texture);

// Creates a single texture in the load thread.
SharedFuture<void> createTextureInLoadThread(
    const AsyncSystem& asyncSystem,
    Model& gltf,
    TextureInfo& textureInfo,
    bool sRGB,
    const std::vector<bool>& imageNeedsMipmaps);

} // namespace

/*static*/ CesiumAsync::Future<void> CesiumGltfTextures::createInWorkerThread(
    const CesiumAsync::AsyncSystem& asyncSystem,
    CesiumGltf::Model& model) {
  // This array is parallel to model.images and indicates whether each image
  // requires mipmaps. An image requires mipmaps if any of its textures have a
  // sampler that will use them.
  std::vector<bool> imageNeedsMipmaps(model.images.size(), false);
  for (const Texture& texture : model.textures) {
    int32_t imageIndex = texture.source;
    if (imageIndex < 0 || imageIndex >= model.images.size()) {
      continue;
    }

    if (!imageNeedsMipmaps[imageIndex]) {
      imageNeedsMipmaps[imageIndex] = doesTextureUseMipmaps(model, texture);
    }
  }

  std::vector<SharedFuture<void>> futures;

  model.forEachPrimitiveInScene(
      -1,
      [&imageNeedsMipmaps, &asyncSystem, &futures](
          Model& gltf,
          Node& node,
          Mesh& mesh,
          MeshPrimitive& primitive,
          const glm::dmat4& transform) {
        if (!isValidPrimitive(gltf, primitive)) {
          return;
        }

        Material* pMaterial =
            Model::getSafe(&gltf.materials, primitive.material);
        if (!pMaterial) {
          // A primitive using the default material will not have any textures.
          return;
        }

        if (pMaterial->pbrMetallicRoughness) {
          if (pMaterial->pbrMetallicRoughness->baseColorTexture) {
            futures.emplace_back(createTextureInLoadThread(
                asyncSystem,
                gltf,
                *pMaterial->pbrMetallicRoughness->baseColorTexture,
                true,
                imageNeedsMipmaps));
          }
          if (pMaterial->pbrMetallicRoughness->metallicRoughnessTexture) {
            futures.emplace_back(createTextureInLoadThread(
                asyncSystem,
                gltf,
                *pMaterial->pbrMetallicRoughness->metallicRoughnessTexture,
                false,
                imageNeedsMipmaps));
          }
        }

        if (pMaterial->emissiveTexture)
          futures.emplace_back(createTextureInLoadThread(
              asyncSystem,
              gltf,
              *pMaterial->emissiveTexture,
              true,
              imageNeedsMipmaps));
        if (pMaterial->normalTexture)
          futures.emplace_back(createTextureInLoadThread(
              asyncSystem,
              gltf,
              *pMaterial->normalTexture,
              false,
              imageNeedsMipmaps));
        if (pMaterial->occlusionTexture)
          futures.emplace_back(createTextureInLoadThread(
              asyncSystem,
              gltf,
              *pMaterial->occlusionTexture,
              false,
              imageNeedsMipmaps));

        // Initialize water mask if needed.
        auto onlyWaterIt = primitive.extras.find("OnlyWater");
        auto onlyLandIt = primitive.extras.find("OnlyLand");
        if (onlyWaterIt != primitive.extras.end() &&
            onlyWaterIt->second.isBool() &&
            onlyLandIt != primitive.extras.end() &&
            onlyLandIt->second.isBool()) {
          bool onlyWater = onlyWaterIt->second.getBoolOrDefault(false);
          bool onlyLand = onlyLandIt->second.getBoolOrDefault(true);

          if (!onlyWater && !onlyLand) {
            // We have to use the water mask
            auto waterMaskTextureIdIt = primitive.extras.find("WaterMaskTex");
            if (waterMaskTextureIdIt != primitive.extras.end() &&
                waterMaskTextureIdIt->second.isInt64()) {
              int32_t waterMaskTextureId = static_cast<int32_t>(
                  waterMaskTextureIdIt->second.getInt64OrDefault(-1));
              TextureInfo waterMaskInfo;
              waterMaskInfo.index = waterMaskTextureId;
              if (waterMaskTextureId >= 0 &&
                  waterMaskTextureId < gltf.textures.size()) {
                futures.emplace_back(createTextureInLoadThread(
                    asyncSystem,
                    gltf,
                    waterMaskInfo,
                    false,
                    imageNeedsMipmaps));
              }
            }
          }
        }
      });

  return asyncSystem.all(std::move(futures));
}

namespace {

bool isSupportedIndexComponentType(int32_t componentType) {
  return componentType == Accessor::ComponentType::UNSIGNED_BYTE ||
         componentType == Accessor::ComponentType::UNSIGNED_SHORT ||
         componentType == Accessor::ComponentType::UNSIGNED_INT;
}

bool isSupportedPrimitiveMode(int32_t primitiveMode) {
  return primitiveMode == MeshPrimitive::Mode::TRIANGLES ||
         primitiveMode == MeshPrimitive::Mode::TRIANGLE_STRIP ||
         primitiveMode == MeshPrimitive::Mode::POINTS;
}

// Determines if a glTF primitive is usable for our purposes.
bool isValidPrimitive(
    const CesiumGltf::Model& gltf,
    const CesiumGltf::MeshPrimitive& primitive) {
  if (!isSupportedPrimitiveMode(primitive.mode)) {
    // This primitive's mode is not supported.
    return false;
  }

  auto positionAccessorIt =
      primitive.attributes.find(VertexAttributeSemantics::POSITION);
  if (positionAccessorIt == primitive.attributes.end()) {
    // This primitive doesn't have a POSITION semantic, so it's not valid.
    return false;
  }

  AccessorView<FVector3f> positionView(gltf, positionAccessorIt->second);
  if (positionView.status() != AccessorViewStatus::Valid) {
    // This primitive's POSITION accessor is invalid, so the primitive is not
    // valid.
    return false;
  }

  const Accessor* pIndexAccessor =
      Model::getSafe(&gltf.accessors, primitive.indices);
  if (pIndexAccessor &&
      !isSupportedIndexComponentType(pIndexAccessor->componentType)) {
    // This primitive's indices are not a supported type, so the primitive is
    // not valid.
    return false;
  }

  return true;
}

bool doesTextureUseMipmaps(const Model& gltf, const Texture& texture) {
  const Sampler& sampler = Model::getSafe(gltf.samplers, texture.sampler);

  switch (sampler.minFilter.value_or(
      CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_LINEAR)) {
  case CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_LINEAR:
  case CesiumGltf::Sampler::MinFilter::LINEAR_MIPMAP_NEAREST:
  case CesiumGltf::Sampler::MinFilter::NEAREST_MIPMAP_LINEAR:
  case CesiumGltf::Sampler::MinFilter::NEAREST_MIPMAP_NEAREST:
    return true;
  default: // LINEAR and NEAREST
    return false;
  }
}

SharedFuture<void> createTextureInLoadThread(
    const AsyncSystem& asyncSystem,
    Model& gltf,
    TextureInfo& textureInfo,
    bool sRGB,
    const std::vector<bool>& imageNeedsMipmaps) {
  Texture* pTexture = Model::getSafe(&gltf.textures, textureInfo.index);
  if (pTexture == nullptr)
    return asyncSystem.createResolvedFuture().share();

  Image* pImage = Model::getSafe(&gltf.images, pTexture->source);
  if (pImage == nullptr)
    return asyncSystem.createResolvedFuture().share();

  check(pTexture->source >= 0 && pTexture->source < imageNeedsMipmaps.size());
  bool needsMips = imageNeedsMipmaps[pTexture->source];

  const ExtensionImageAssetUnreal& extension =
      ExtensionImageAssetUnreal::getOrCreate(
          asyncSystem,
          *pImage->pCesium,
          sRGB,
          needsMips,
          std::nullopt);

  return extension.getFuture();
}

} // namespace
