// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumCommon.h"
#include "Engine/Texture.h"
#include "TextureResource.h"
#include <CesiumGltf/ImageCesium.h>
#include <CesiumGltf/SharedAssetDepot.h>

/**
 * The base class for Cesium texture resources, making Cesium's texture data
 * available to Unreal's RHI. The actual creation of the RHI texture is deferred
 * to a pure virtual method, `InitializeTextureRHI`.
 */
class FCesiumTextureResourceBase : public FTextureResource {
public:
  FCesiumTextureResourceBase(
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

/**
 * A Cesium texture resource that uses an already-created `FRHITexture`. This is
 * used when `GRHISupportsAsyncTextureCreation` is true and so we were already
 * able to create the FRHITexture in a worker thread. It is also used when a
 * single glTF `Image` is referenced by multiple glTF `Texture` instances. We
 * only need one `FRHITexture` is this case, but we need multiple
 * `FTextureResource` instances to support the different sampler settings that
 * are likely used in the different textures.
 */
class FCesiumUseExistingTextureResource : public FCesiumTextureResourceBase {
public:
  FCesiumUseExistingTextureResource(
      FTextureRHIRef existingTexture,
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

  FCesiumUseExistingTextureResource(
      FTextureResource* pExistingTexture,
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

protected:
  virtual FTextureRHIRef InitializeTextureRHI() override;

private:
  FTextureResource* _pExistingTexture;
};

/**
 * A Cesium texture resource that creates an `FRHITexture` from a glTF
 * `ImageCesium` when `InitRHI` is called from the render thread. When
 * `GRHISupportsAsyncTextureCreation` is false (everywhere but Direct3D), we can
 * only create a `FRHITexture` on the render thread, so this is the code that
 * does it.
 */
class FCesiumCreateNewTextureResource : public FCesiumTextureResourceBase {
public:
  FCesiumCreateNewTextureResource(
      CesiumGltf::ImageCesium&& image,
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

protected:
  virtual FTextureRHIRef InitializeTextureRHI() override;

private:
  CesiumGltf::ImageCesium _image;
};
