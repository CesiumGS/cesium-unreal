// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/Model.h"
#include "CesiumGltf/SharedAssetDepot.h"
#include "CesiumMetadataValueType.h"
#include "CesiumTextureResource.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureDefines.h"
#include "RHI.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Templates/UniquePtr.h"
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/ReferenceCounted.h>

namespace CesiumGltf {
struct ImageCesium;
struct Texture;
} // namespace CesiumGltf

namespace CesiumTextureUtility {

// A slightly roundabout way to a hold a UTexture2D.
//
// We can't let Unreal's garbage collector be exclusively responsible for the
// lifetime of our textures because it doesn't run often enough (and not at all
// in the Editor). And we also need shared ownership of UTexture2Ds when a tile
// is "upsampled" from its parent for raster overlays. So this class allows us
// to control the lifetime of a UTexture2D via reference counting.
//
// Yes, this means we're controlling the lifetime of a garbage collected
// `UTexture2D` object via reference counting.
//
// Instances of this class are created whenever we create a UTexture2D. A
// pointer to the instance is held in `LoadedTextureResult` as well as in a
// private extension added to the glTF `Texture` from which the UTexture2D was
// created.
struct ReferenceCountedUnrealTexture
    : CesiumUtility::ReferenceCountedThreadSafe<ReferenceCountedUnrealTexture> {
  ReferenceCountedUnrealTexture() noexcept;
  ~ReferenceCountedUnrealTexture() noexcept;

  // The texture game object, once it's created.
  TObjectPtr<UTexture2D> getUnrealTexture() const;
  void setUnrealTexture(const TObjectPtr<UTexture2D>& p);

  // The renderer / RHI FTextureResource holding the pixel data.
  const FCesiumTextureResourceUniquePtr& getTextureResource() const;
  FCesiumTextureResourceUniquePtr& getTextureResource();
  void setTextureResource(FCesiumTextureResourceUniquePtr&& p);

private:
  TObjectPtr<UTexture2D> _pUnrealTexture;
  FCesiumTextureResourceUniquePtr _pTextureResource;
};

/**
 * @brief Half-loaded Unreal texture with info on how to finish loading the
 * texture on the game thread and render thread.
 */
struct LoadedTextureResult {
  TextureAddress addressX;
  TextureAddress addressY;
  TextureFilter filter;
  TextureGroup group;
  bool sRGB{true};

  // The index of the `CesiumGltf::Texture` instance with the glTF. Or -1 if
  // this result wasn't created from a texture in a glTF.
  int64_t textureIndex = -1;

  // The UTexture2D that has already been created, if any.
  CesiumUtility::IntrusivePointer<ReferenceCountedUnrealTexture> pTexture;
};

/**
 * Does the asynchronous part of renderer resource preparation for a `Texture`
 * in a glTF. Should be called in a worker thread.
 *
 * The `cesium.pixelData` will be removed from the image associated with the
 * texture so that it can be passed to Unreal's renderer thread without copying
 * it.
 *
 * @param model The glTF Model for which to load this texture.
 * @param texture The glTF Texture to load. This parameter is non-const because
 * a private extension will be added to this `Texture` in order to track the
 * associated Unreal texture.
 * @param sRGB True if the texture should be treated as sRGB; false if it should
 * be treated as linear.
 * @param textureResources Unreal RHI texture resources that have already been
 * created for this model. This array must have the same size as `model`'s
 * {@link CesiumGltf::Model::images}, and all pointers must be initialized to
 * nullptr before the first call to `loadTextureFromModelAnyThreadPart` during
 * the glTF load process.
 */
TUniquePtr<LoadedTextureResult> loadTextureFromModelAnyThreadPart(
    CesiumGltf::Model& model,
    CesiumGltf::Texture& texture,
    bool sRGB);

/**
 * Does the asynchronous part of renderer resource preparation for a glTF
 * `Image` with the given `Sampler` settings.
 *
 * The `cesium.pixelData` will be removed from the image so that it can be
 * passed to Unreal's renderer thread without copying it.
 *
 * @param image The glTF image for each to create a texture.
 * @param sampler The sampler settings to use with the texture.
 * @param sRGB True if the texture should be treated as sRGB; false if it should
 * be treated as linear.
 * @param pExistingImageResource An existing RHI texture resource that has been
 * created for this image, or nullptr if one hasn't been created yet. When this
 * parameter is not nullptr, the provided image's `pixelData` is not required
 * and can be empty.
 */
TUniquePtr<LoadedTextureResult> loadTextureFromImageAndSamplerAnyThreadPart(
    const CesiumGltf::ImageCesium& image,
    const CesiumGltf::Sampler& sampler,
    bool sRGB);

/**
 * @brief Does the asynchronous part of renderer resource preparation for
 * a texture. The given image _must_ be prepared before calling this method by
 * calling {@link ExtensionImageCesiumUnreal::GetOrCreate} and then waiting
 * for {@link ExtensionImageCesiumUnreal::getFuture} to resolve. This method
 * should be called in a background thread.
 *
 * @param imageCesium The image.
 * @param addressX The X addressing mode.
 * @param addressY The Y addressing mode.
 * @param filter The sampler filtering to use for this texture.
 * @param useMipMapsIfAvailable true to use this image's mipmaps for sampling,
 * if they exist; false to ignore any mipmaps that might be present.
 * @param group The texture group of this texture.
 * @param sRGB Whether this texture uses a sRGB color space.
 * @param overridePixelFormat The explicit pixel format to use. If std::nullopt,
 * the pixel format is inferred from the image.
 * @return The loaded texture.
 */
TUniquePtr<LoadedTextureResult> loadTextureAnyThreadPart(
    const CesiumGltf::ImageCesium& image,
    TextureAddress addressX,
    TextureAddress addressY,
    TextureFilter filter,
    bool useMipMapsIfAvailable,
    TextureGroup group,
    bool sRGB,
    std::optional<EPixelFormat> overridePixelFormat);

/**
 * @brief Does the main-thread part of render resource preparation for this
 * image and queues up any required render-thread tasks to finish preparing the
 * image.
 *
 * @param model The model with which this texture is associated. This is used to
 * store a pointer to the created texture in an extension on the glTF texture so
 * that it can be reused later.
 * @param pHalfLoadedTexture The half-loaded renderer texture.
 * @return The Unreal texture result.
 */
CesiumUtility::IntrusivePointer<ReferenceCountedUnrealTexture>
loadTextureGameThreadPart(
    CesiumGltf::Model& model,
    LoadedTextureResult* pHalfLoadedTexture);

/**
 * @brief Does the main-thread part of render resource preparation for this
 * image and queues up any required render-thread tasks to finish preparing the
 * image.
 *
 * @param pHalfLoadedTexture The half-loaded renderer texture.
 * @return The Unreal texture result.
 */
CesiumUtility::IntrusivePointer<ReferenceCountedUnrealTexture>
loadTextureGameThreadPart(LoadedTextureResult* pHalfLoadedTexture);

/**
 * @brief Convert a glTF {@link CesiumGltf::Sampler::WrapS} value to an Unreal
 * `TextureAddress` value.
 *
 * @param wrapS The glTF wrapS value.
 * @returns The Unreal equivalent, or `TextureAddress::TA_Wrap` if the glTF
 * value is unknown or invalid.
 */
TextureAddress convertGltfWrapSToUnreal(int32_t wrapS);

/**
 * @brief Convert a glTF {@link CesiumGltf::Sampler::WrapT} value to an Unreal
 * `TextureAddress` value.
 *
 * @param wrapT The glTF wrapT value.
 * @returns The Unreal equivalent, or `TextureAddress::TA_Wrap` if the glTF
 * value is unknown or invalid.
 */
TextureAddress convertGltfWrapTToUnreal(int32_t wrapT);

std::optional<EPixelFormat> getPixelFormatForImageCesium(
    const CesiumGltf::ImageCesium& imageCesium,
    const std::optional<EPixelFormat> overridePixelFormat);

} // namespace CesiumTextureUtility
