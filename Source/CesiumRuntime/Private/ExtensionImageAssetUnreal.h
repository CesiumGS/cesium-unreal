// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumTextureResource.h"
#include "PixelFormat.h"
#include "Templates/SharedPointer.h"
#include <CesiumAsync/SharedFuture.h>
#include <optional>

namespace CesiumGltf {
struct ImageAsset;
}

/**
 * @brief An extension attached to an ImageAsset in order to hold
 * Unreal-specific information about it.
 *
 * ImageAsset instances are shared between multiple textures on a single model,
 * and even between models in some cases, but we strive to have only one copy of
 * the image bytes in GPU memory.
 *
 * The Unreal / GPU resource is held in `pTextureResource`, which may be either
 * a `FCesiumCreateNewTextureResource` or a `FCesiumUseExistingTextureResource`
 * depending on how it was created. We'll never actually sample directly from
 * this resource, however. Instead, a separate
 * `FCesiumUseExistingTextureResource` will be created for each glTF Texture
 * that references this image, and it will point to the instance managed by this
 * extension.
 *
 * Because we'll never be sampling from this texture resource, the texture
 * filtering and addressing parameters have default values.
 */
struct ExtensionImageAssetUnreal {
  static inline constexpr const char* TypeName = "ExtensionImageAssetUnreal";
  static inline constexpr const char* ExtensionName =
      "PRIVATE_ImageAsset_Unreal";

  /**
   * @brief Gets an Unreal texture resource from the given `ImageAsset`,
   * creating it if necessary.
   *
   * When this function is called for the first time on a particular
   * `ImageAsset`, the asynchronous process to create an Unreal
   * `FTextureResource` from it is kicked off. On successive invocations
   * (perhaps from other threads), the existing instance is returned. It is safe
   * to call this method on the same `ImageAsset` instance from multiple
   * threads simultaneously as long as no other thread is modifying the instance
   * at the same time.
   *
   * To determine if the asynchronous `FTextureResource` creation process has
   * completed, use {@link getFuture}.
   */
  static const ExtensionImageAssetUnreal& getOrCreate(
      const CesiumAsync::AsyncSystem& asyncSystem,
      CesiumGltf::ImageAsset& imageCesium,
      bool sRGB,
      bool needsMipMaps,
      const std::optional<EPixelFormat>& overridePixelFormat);

  /**
   * Constructs a new instance.
   *
   * @param future The future that will resolve when loading of the
   * {@link getTextureResource} is complete.
   */
  ExtensionImageAssetUnreal(const CesiumAsync::SharedFuture<void>& future);

  /**
   * Gets the created texture resource. This resource should not be accessed or
   * used before the future returned by {@link getFuture} resolves.
   */
  const TSharedPtr<FCesiumTextureResource>& getTextureResource() const;

  /**
   * Gets the future that will resolve when loading of the
   * {@link getTextureResource} is complete. This future will not reject.
   */
  CesiumAsync::SharedFuture<void>& getFuture();

  /**
   * Gets the future that will resolve when loading of the
   * {@link getTextureResource} is complete. This future will not reject.
   */
  const CesiumAsync::SharedFuture<void>& getFuture() const;

private:
  TSharedPtr<FCesiumTextureResource> _pTextureResource;
  CesiumAsync::SharedFuture<void> _futureCreateResource;
};
