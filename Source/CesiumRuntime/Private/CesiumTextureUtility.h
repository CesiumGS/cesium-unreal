// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGltf/Model.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Templates/UniquePtr.h"
#include <optional>


class CesiumTextureUtility {
public:
  struct LoadedTextureResult {
    TUniquePtr<FTexturePlatformData> pTextureData;
    TextureAddress addressX;
    TextureAddress addressY;
    TextureFilter filter;
    UTexture2D* pTexture{nullptr};
  };

  // TODO: documentation
  static TUniquePtr<LoadedTextureResult> loadTextureAnyThreadPart(
      const CesiumGltf::ImageCesium& image,
      const TextureAddress& addressX,
      const TextureAddress& addressY,
      const TextureFilter& filter);

  static TUniquePtr<LoadedTextureResult> loadTextureAnyThreadPart(
      const CesiumGltf::Model& model,
      const CesiumGltf::Texture& texture);

  static UTexture2D*
  loadTextureGameThreadPart(LoadedTextureResult* pHalfLoadedTexture);
};
