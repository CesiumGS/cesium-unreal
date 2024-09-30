// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumTextureResource.h"
#include "PixelFormat.h"
#include "Templates/SharedPointer.h"
#include <CesiumAsync/SharedFuture.h>
#include <optional>

namespace CesiumGltf {
struct ImageCesium;
}

/**
 * @brief An extension attached to an ImageCesium in order to hold
 * Unreal-specific information about it.
 *
 * ImageCesium instances are shared between multiple textures on a single model,
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
struct ExtensionImageCesiumUnreal {
  static inline constexpr const char* TypeName = "ExtensionImageCesiumUnreal";
  static inline constexpr const char* ExtensionName =
      "PRIVATE_ImageCesium_Unreal";

  /**
   * @brief Gets an Unreal texture resource from the given `ImageCesium`,
   * creating it if necessary.
   *
   * When this function is called for the first time on a particular
   * `ImageCesium`, the asynchronous process to create an Unreal
   * `FTextureResource` from it is kicked off. On successive invocations
   * (perhaps from other threads), the existing instance is returned. It is safe
   * to call this method on the same `ImageCesium` instance from multiple
   * threads simultaneously.
   *
   * To determine if the asynchronous `FTextureResource` creation process has
   * completed, use {@link getFuture}.
   */
  static const ExtensionImageCesiumUnreal& getOrCreate(
      const CesiumAsync::AsyncSystem& asyncSystem,
      CesiumGltf::ImageCesium& imageCesium,
      bool sRGB,
      bool needsMipMaps,
      const std::optional<EPixelFormat>& overridePixelFormat);

  ExtensionImageCesiumUnreal(const CesiumAsync::SharedFuture<void>& future);

  const TSharedPtr<FCesiumTextureResource>& getTextureResource() const;
  CesiumAsync::SharedFuture<void>& getFuture();
  const CesiumAsync::SharedFuture<void>& getFuture() const;

private:
  TSharedPtr<FCesiumTextureResource> _pTextureResource;
  CesiumAsync::SharedFuture<void> _futureCreateResource;
};
