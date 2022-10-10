// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/Model.h"
#include "CesiumMetadataValueType.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureDefines.h"
#include "Templates/UniquePtr.h"
#include <optional>
#include <variant>

namespace CesiumGltf {
struct ImageCesium;
struct Texture;
} // namespace CesiumGltf

namespace CesiumTextureUtility {
/**
 * @brief A texture that has already been asynchronously created.
 */
struct AsyncCreatedTexture {
  FTexture2DRHIRef rhiTextureRef{};
};

/**
 * @brief A pointer to a glTF image. This image will be cached and used on the
 * game thread and render thread during texture creation.
 *
 * WARNING: Do not use this form of texture creation if the given pointer will
 * be invalidated before the render-thread texture preparation work is done.
 */
struct GltfImagePtr {
  CesiumGltf::ImageCesium* pImage;
};

/**
 * @brief An index to an image that can be looked up later in the corresponding
 * glTF.
 */
struct GltfImageIndex {
  int32_t index = -1;
  GltfImagePtr resolveImage(const CesiumGltf::Model& model) const;
};

/**
 * @brief An embedded image resource.
 */
struct EmbeddedImageSource {
  CesiumGltf::ImageCesium image;
};

/**
 * @brief This indicates that the image mips are stored in the
 * FTexturePlatformData and expect a standard, Unreal texture construction.
 *
 * WARNING: Unreal's default texture creation method (via
 * UTexture::UpdateResource) requires an extra memcpy on the game thread and
 * synchronous texture creation on the render thread.
 */
struct LegacyTextureSource {};

/**
 * @brief The texture source that should be used to create or finalize an
 * Unreal texture.
 */
typedef std::variant<
    AsyncCreatedTexture,
    GltfImagePtr,
    GltfImageIndex,
    EmbeddedImageSource,
    LegacyTextureSource>
    CesiumTextureSource;

/**
 * @brief Half-loaded Unreal texture with info on how to finish loading the
 * texture on the game thread and render thread.
 */
struct LoadedTextureResult {
  TUniquePtr<FTexturePlatformData> pTextureData;
  TextureAddress addressX;
  TextureAddress addressY;
  TextureFilter filter;
  TextureGroup group;
  bool generateMipMaps;
  bool sRGB{true};
  TWeakObjectPtr<UTexture2D> pTexture;
  CesiumTextureSource textureSource;
};

TUniquePtr<FTexturePlatformData>
createTexturePlatformData(int32 sizeX, int32 sizeY, EPixelFormat format);

/**
 * @brief Does the asynchronous part of renderer resource preparation for this
 * image. Should be called in a background thread. May generate mip-maps for
 * this image within the given glTF, if needed.
 *
 * @param imageSource The source for this image. This function may add mip-maps
 * to the image if needed.
 * @param addressX The X addressing mode.
 * @param addressY The Y addressing mode.
 * @param filter The sampler filtering to use for this texture.
 * @param group The texture group of this texture.
 * @param generateMipMaps Whether to generate a mipmap for this image.
 * @param sRGB Whether this texture uses a sRGB color space.
 * @return The loaded texture.
 */
TUniquePtr<LoadedTextureResult> loadTextureAnyThreadPart(
    CesiumTextureSource&& imageSource,
    const TextureAddress& addressX,
    const TextureAddress& addressY,
    const TextureFilter& filter,
    const TextureGroup& group,
    bool generateMipMaps,
    bool sRGB);

/**
 * @brief Does the asynchronous part of renderer resource preparation for this
 * image. Should be called in a background thread. May generate mip-maps for
 * this image within the given glTF, if needed.
 *
 * @param model The model.
 * @param texture The texture to load.
 * @param sRGB Whether this texture uses a sRGB color space.
 * @return The loaded texture.
 */
TUniquePtr<LoadedTextureResult> loadTextureAnyThreadPart(
    CesiumGltf::Model& model,
    const CesiumGltf::Texture& texture,
    bool sRGB);

/**
 * @brief Does the main-thread part of render resource preparation for this
 * image and queues up any required render-thread tasks to finish preparing the
 * image.
 *
 * NOTE: Assumes LoadedTextureResult::textureSource is not GltfImageIndex.
 * Use GltfImageIndex::resolve(...) to convert to a GltfImagePtr.
 *
 * @param pHalfLoadedTexture The half-loaded renderer texture.
 * @return The Unreal texture result.
 */
UTexture2D* loadTextureGameThreadPart(LoadedTextureResult* pHalfLoadedTexture);

/**
 * @brief Does the main-thread part of render resource preparation for this
 * image and queues up any required render-thread tasks to finish preparing the
 * image. Resolves the textureSource to a GltfImagePtr, if it is currently
 * GltfImageIndex.
 *
 * @param model The glTF model, containing the image.
 * @param pHalfLoadedTexture The half-loaded renderer texture.
 * @return The Unreal texture result.
 */
UTexture2D* loadTextureGameThreadPart(
    const CesiumGltf::Model& model,
    LoadedTextureResult* pHalfLoadedTexture);

void destroyHalfLoadedTexture(LoadedTextureResult& halfLoaded);
void destroyTexture(UTexture* pTexture);
} // namespace CesiumTextureUtility
