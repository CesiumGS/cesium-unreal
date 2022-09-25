// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/Model.h"
#include "CesiumMetadataValueType.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureDefines.h"
#include "Templates/UniquePtr.h"
#include <optional>

namespace CesiumGltf {
struct ImageCesium;
struct Texture;
} // namespace CesiumGltf

namespace CesiumTextureUtility {
struct LoadedTextureResult {
  TUniquePtr<FTexturePlatformData> pTextureData;
  TextureAddress addressX;
  TextureAddress addressY;
  TextureFilter filter;
  TextureGroup group;
  bool sRGB{true};
  TWeakObjectPtr<UTexture2D> pTexture;

  // If GRHISupportsAsyncTextureCreation is true, this will contain an
  // asynchronously created RHI texture.
  FTexture2DRHIRef rhiTextureRef{};

  // If GRHISupportsAsyncTextureCreation is false, the in-memory glTF mips will
  // be copied directly into GPU memory on the render thread.
  // WARNING: Ensure that this pointer is kept valid for the duration of the
  // render thread resource creation.
  const CesiumGltf::ImageCesium* pCesiumImage{};
};

TUniquePtr<FTexturePlatformData>
createTexturePlatformData(int32 sizeX, int32 sizeY, EPixelFormat format);

// TODO: documentation
TUniquePtr<LoadedTextureResult> loadTextureAnyThreadPart(
    const CesiumGltf::ImageCesium& image,
    const TextureAddress& addressX,
    const TextureAddress& addressY,
    const TextureFilter& filter,
    const TextureGroup& group,
    bool generateMipMaps,
    bool sRGB);

TUniquePtr<LoadedTextureResult> loadTextureAnyThreadPart(
    const CesiumGltf::Model& model,
    const CesiumGltf::Texture& texture,
    bool sRGB);

UTexture2D* loadTextureGameThreadPart(LoadedTextureResult* pHalfLoadedTexture);

void destroyTexture(UTexture* pTexture);
} // namespace CesiumTextureUtility
