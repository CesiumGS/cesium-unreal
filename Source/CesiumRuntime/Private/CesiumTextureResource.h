// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumCommon.h"
#include "Engine/Texture.h"
#include "TextureResource.h"
#include <CesiumGltf/ImageCesium.h>
#include <CesiumGltf/SharedAssetDepot.h>

class FCesiumTextureResource;

struct FCesiumTextureResourceDeleter {
  void operator()(FCesiumTextureResource* p);
};

using FCesiumTextureResourceUniquePtr =
    TUniquePtr<FCesiumTextureResource, FCesiumTextureResourceDeleter>;

/**
 * The base class for Cesium texture resources, making Cesium's texture data
 * available to Unreal's RHI. The actual creation of the RHI texture is deferred
 * to a pure virtual method, `InitializeTextureRHI`.
 */
class FCesiumTextureResource : public FTextureResource {
public:
  /**
   * Create a new FCesiumTextureResource from an ImageCesium and the given
   * sampling parameters. This method is intended to be called from a worker
   * thread, not from the game or render thread.
   *
   * @param imageCesium The image data from which to create the texture
   * resource. After this method returns, the `pixelData` will be empty, and
   * `sizeBytes` will be set to its previous size.
   * @param textureGroup The texture group in which to create this texture.
   * @param overridePixelFormat Overrides the pixel format. If std::nullopt, the
   * format is inferred from the `ImageCesium`.
   * @param filter The texture filtering to use when sampling this texture.
   * @param addressX The X texture addressing mode to use when sampling this
   * texture.
   * @param addressY The Y texture addressing mode to use when sampling this
   * texture.
   * @param sRGB True if the image data stored in this texture should be treated
   * as sRGB.
   * @param needsMipMaps True if this texture requires mipmaps. They will be
   * generated if they don't already exist.
   * @return The created texture resource, or nullptr if a texture could not be
   * created.
   */
  static FCesiumTextureResourceUniquePtr CreateNew(
      CesiumGltf::ImageCesium& imageCesium,
      TextureGroup textureGroup,
      const std::optional<EPixelFormat>& overridePixelFormat,
      TextureFilter filter,
      TextureAddress addressX,
      TextureAddress addressY,
      bool sRGB,
      bool needsMipMaps);

  /**
   * Create a new FCesiumTextureResource wrapping an existing one and providing
   * new sampling parameters. This method is intended to be called from a worker
   * thread, not from the game or render thread.
   */
  static FCesiumTextureResourceUniquePtr CreateWrapped(
      const TSharedPtr<FCesiumTextureResource>& pExistingResource,
      TextureGroup textureGroup,
      TextureFilter filter,
      TextureAddress addressX,
      TextureAddress addressY,
      bool sRGB,
      bool useMipMapsIfAvailable);

  /**
   * Destroys an FCesiumTextureResource. Unreal TextureResources must be
   * destroyed on the render thread, so it is important not to call `delete`
   * directly.
   *
   * \param p
   */
  static void Destroy(FCesiumTextureResource* p);

  FCesiumTextureResource(
      TextureGroup textureGroup,
      uint32 width,
      uint32 height,
      EPixelFormat format,
      TextureFilter filter,
      TextureAddress addressX,
      TextureAddress addressY,
      bool sRGB,
      bool useMipsIfAvailable,
      uint32 extData);

  uint32 GetSizeX() const override { return this->_width; }
  uint32 GetSizeY() const override { return this->_height; }

#if ENGINE_VERSION_5_3_OR_HIGHER
  virtual void InitRHI(FRHICommandListBase& RHICmdList) override;
#else
  virtual void InitRHI() override;
#endif
  virtual void ReleaseRHI() override;

#if STATS
  static FName TextureGroupStatFNames[TEXTUREGROUP_MAX];
#endif

protected:
  virtual FTextureRHIRef InitializeTextureRHI() = 0;

  TextureGroup _textureGroup;
  uint32 _width;
  uint32 _height;
  EPixelFormat _format;
  ESamplerFilter _filter;
  ESamplerAddressMode _addressX;
  ESamplerAddressMode _addressY;
  bool _useMipsIfAvailable;
  uint32 _platformExtData;
  FName _lodGroupStatName;
  uint64 _textureSize;
};
