// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumGltfTextures.h"
#include "CesiumRuntime.h"
#include "CesiumTextureResource.h"
#include "CesiumTextureUtility.h"
#include <CesiumGltf/AccessorView.h>
#include <CesiumGltf/AttributeSemantics.h>
#include <CesiumGltf/Model.h>
#include <CesiumGltfReader/GltfReader.h>

using namespace CesiumAsync;
using namespace CesiumGltf;

namespace {

// Determines if a glTF primitive is usable for our purposes.
bool isValidPrimitive(Model& gltf, MeshPrimitive& primitive);

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

/*static*/ CesiumAsync::Future<void> CesiumGltfTextures::createInLoadThread(
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
    CesiumGltf::Model& gltf,
    CesiumGltf::MeshPrimitive& primitive) {
  if (!isSupportedPrimitiveMode(primitive.mode)) {
    // This primitive's mode is not supported.
    return false;
  }

  auto positionAccessorIt =
      primitive.attributes.find(AttributeSemantics::POSITION);
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

  Accessor* pIndexAccessor = Model::getSafe(&gltf.accessors, primitive.indices);
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

struct ExtensionUnrealTextureResource {
  static inline constexpr const char* TypeName =
      "ExtensionUnrealTextureResource";
  static inline constexpr const char* ExtensionName =
      "PRIVATE_unreal_texture_resource";

  ExtensionUnrealTextureResource() {}

  TSharedPtr<FCesiumTextureResourceBase> pTextureResource = nullptr;
  std::optional<CesiumAsync::SharedFuture<void>> createFuture = std::nullopt;
};

std::mutex textureResourceMutex;

// Returns a Future that will resolve when the image is loaded. It _may_ also
// return a Promise, in which case the calling thread is responsible for doing
// the loading and should resolve the Promise when it's done.
std::pair<SharedFuture<void>, std::optional<Promise<void>>>
getOrCreateImageFuture(const AsyncSystem& asyncSystem, Image& image) {
  std::scoped_lock lock(textureResourceMutex);

  ExtensionUnrealTextureResource& extension =
      image.cesium->addExtension<ExtensionUnrealTextureResource>();
  if (extension.createFuture) {
    // Another thread is already working on this image.
    return {*extension.createFuture, std::nullopt};
  } else {
    // This thread will work on this image.
    Promise<void> promise = asyncSystem.createPromise<void>();
    return {promise.getFuture().share(), promise};
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

  auto [future, maybePromise] = getOrCreateImageFuture(asyncSystem, *pImage);
  if (!maybePromise) {
    // Another thread is already loading this image.
    return future;
  }

  // Proceed to load the image in this thread.
  ImageCesium& cesium = *pImage->cesium;

  if (needsMips && !cesium.pixelData.empty()) {
    std::optional<std::string> errorMessage =
        CesiumGltfReader::GltfReader::generateMipMaps(cesium);
    if (errorMessage) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT("%s"),
          UTF8_TO_TCHAR(errorMessage->c_str()));
    }
  }

  CesiumTextureUtility::loadTextureFromModelAnyThreadPart(
      gltf,
      *pTexture,
      sRGB);

  maybePromise->resolve();

  return future;
}

} // namespace
