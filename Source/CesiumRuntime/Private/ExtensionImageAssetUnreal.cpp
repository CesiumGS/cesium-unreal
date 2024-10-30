#include "ExtensionImageAssetUnreal.h"
#include "CesiumRuntime.h"
#include "CesiumTextureUtility.h"
#include <CesiumGltf/ImageAsset.h>
#include <CesiumGltfReader/GltfReader.h>

using namespace CesiumAsync;
using namespace CesiumGltfReader;

namespace {

std::mutex createExtensionMutex;

std::pair<ExtensionImageAssetUnreal&, std::optional<Promise<void>>>
getOrCreateImageFuture(
    const AsyncSystem& asyncSystem,
    CesiumGltf::ImageAsset& imageCesium);

} // namespace

/*static*/ const ExtensionImageAssetUnreal&
ExtensionImageAssetUnreal::getOrCreate(
    const CesiumAsync::AsyncSystem& asyncSystem,
    CesiumGltf::ImageAsset& imageCesium,
    bool sRGB,
    bool needsMipMaps,
    const std::optional<EPixelFormat>& overridePixelFormat) {
  auto [extension, maybePromise] =
      getOrCreateImageFuture(asyncSystem, imageCesium);
  if (!maybePromise) {
    // Another thread is already working on this image.
    return extension;
  }

  // Proceed to load the image in this thread.
  TUniquePtr<FCesiumTextureResource, FCesiumTextureResourceDeleter> pResource =
      FCesiumTextureResource::CreateNew(
          imageCesium,
          TextureGroup::TEXTUREGROUP_World,
          overridePixelFormat,
          TextureFilter::TF_Default,
          TextureAddress::TA_Clamp,
          TextureAddress::TA_Clamp,
          sRGB,
          needsMipMaps);

  extension._pTextureResource =
      MakeShareable(pResource.Release(), [](FCesiumTextureResource* p) {
        FCesiumTextureResource ::Destroy(p);
      });

  maybePromise->resolve();

  return extension;
}

ExtensionImageAssetUnreal::ExtensionImageAssetUnreal(
    const CesiumAsync::SharedFuture<void>& future)
    : _pTextureResource(nullptr), _futureCreateResource(future) {}

const TSharedPtr<FCesiumTextureResource>&
ExtensionImageAssetUnreal::getTextureResource() const {
  return this->_pTextureResource;
}

CesiumAsync::SharedFuture<void>& ExtensionImageAssetUnreal::getFuture() {
  return this->_futureCreateResource;
}

const CesiumAsync::SharedFuture<void>&
ExtensionImageAssetUnreal::getFuture() const {
  return this->_futureCreateResource;
}

namespace {

// Returns the ExtensionImageAssetUnreal, which is created if it does not
// already exist. It _may_ also return a Promise, in which case the calling
// thread is responsible for doing the loading and should resolve the Promise
// when it's done.
std::pair<ExtensionImageAssetUnreal&, std::optional<Promise<void>>>
getOrCreateImageFuture(
    const AsyncSystem& asyncSystem,
    CesiumGltf::ImageAsset& imageCesium) {
  std::scoped_lock lock(createExtensionMutex);

  ExtensionImageAssetUnreal* pExtension =
      imageCesium.getExtension<ExtensionImageAssetUnreal>();
  if (!pExtension) {
    // This thread will work on this image.
    Promise<void> promise = asyncSystem.createPromise<void>();
    ExtensionImageAssetUnreal& extension =
        imageCesium.addExtension<ExtensionImageAssetUnreal>(
            promise.getFuture().share());
    return {extension, std::move(promise)};
  } else {
    // Another thread is already working on this image.
    return {*pExtension, std::nullopt};
  }
}

} // namespace
