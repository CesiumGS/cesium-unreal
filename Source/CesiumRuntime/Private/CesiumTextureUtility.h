// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/Model.h"
#include "CesiumMetadataValueType.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include <optional>

namespace CesiumGltf {
struct ImageCesium;
} // namespace CesiumGltf

namespace CesiumTextureUtility {
struct LoadedTextureResult {
  FTexturePlatformData* pTextureData;
  TextureAddress addressX;
  TextureAddress addressY;
  TextureFilter filter;
  UTexture2D* pTexture;
};

FTexturePlatformData*
createTexturePlatformData(int32 sizeX, int32 sizeY, EPixelFormat format);

// TODO: documentation
LoadedTextureResult* loadTextureAnyThreadPart(
    const CesiumGltf::ImageCesium& image,
    const TextureAddress& addressX,
    const TextureAddress& addressY,
    const TextureFilter& filter);

LoadedTextureResult* loadTextureAnyThreadPart(
    const CesiumGltf::Model& model,
    const CesiumGltf::Texture& texture);

bool loadTextureGameThreadPart(LoadedTextureResult* pHalfLoadedTexture);
} // namespace CesiumTextureUtility
