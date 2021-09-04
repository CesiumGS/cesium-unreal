// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/Model.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include <optional>

class CesiumTextureUtility {
public:
  struct LoadedTextureResult {
    FTexturePlatformData* pTextureData;
    TextureAddress addressX;
    TextureAddress addressY;
    TextureFilter filter;
    UTexture2D* pTexture;
  };

  // TODO: documentation
  static LoadedTextureResult* loadTextureAnyThreadPart(
      const CesiumGltf::ImageCesium& image,
      const TextureAddress& addressX,
      const TextureAddress& addressY,
      const TextureFilter& filter);

  static LoadedTextureResult* loadTextureAnyThreadPart(
      const CesiumGltf::Model& model,
      const CesiumGltf::Texture& texture);

  static bool
  loadTextureGameThreadPart(LoadedTextureResult* pHalfLoadedTexture);
};
