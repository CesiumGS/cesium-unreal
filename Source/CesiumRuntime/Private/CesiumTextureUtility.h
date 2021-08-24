// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/Model.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include <optional>

class CesiumTextureUtility {
public:
  struct HalfLoadedTexture {
    FTexturePlatformData* pTextureData;
    TextureAddress addressX;
    TextureAddress addressY;
    TextureFilter filter;
  };

  // TODO: documentation
  static HalfLoadedTexture* loadTextureAnyThreadPart(
      const CesiumGltf::ImageCesium& image,
      const TextureAddress& addressX,
      const TextureAddress& addressY,
      const TextureFilter& filter);

  static HalfLoadedTexture* loadTextureAnyThreadPart(
      const CesiumGltf::Model& model,
      const CesiumGltf::Texture& texture);

  static UTexture2D*
  loadTextureGameThreadPart(HalfLoadedTexture* pHalfLoadedTexture);
};
