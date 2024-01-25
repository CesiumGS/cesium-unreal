#pragma once

#include <CesiumGltf/ImageCesium.h>
#include <CesiumUtility/IntrusivePointer.h>
#include <CesiumUtility/ReferenceCountedThreadSafe.h>

struct ImageCesiumRefCounted
    : public CesiumUtility::ReferenceCountedThreadSafe<ImageCesiumRefCounted> {
  CesiumGltf::ImageCesium image;
};

struct ExtensionUnrealImageData {
  static inline constexpr const char* TypeName = "ExtensionUnrealImageData";
  static inline constexpr const char* ExtensionName =
      "PRIVATE_unreal_image_data";

  CesiumUtility::IntrusivePointer<ImageCesiumRefCounted> pImage;
};
