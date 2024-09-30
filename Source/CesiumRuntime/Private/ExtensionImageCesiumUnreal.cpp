#include "ExtensionImageCesiumUnreal.h"
#include "CesiumRuntime.h"
#include "CesiumTextureUtility.h"
#include <CesiumGltf/ImageCesium.h>
#include <CesiumGltfReader/GltfReader.h>

using namespace CesiumAsync;
using namespace CesiumGltf;
using namespace CesiumGltfReader;

namespace {

std::mutex createExtensionMutex;

std::pair<ExtensionImageCesiumUnreal&, std::optional<Promise<void>>>
getOrCreateImageFuture(
    const AsyncSystem& asyncSystem,
    ImageCesium& imageCesium);

} // namespace

/*static*/ const ExtensionImageCesiumUnreal&
ExtensionImageCesiumUnreal::GetOrCreate(
    const CesiumAsync::AsyncSystem& asyncSystem,
    CesiumGltf::ImageCesium& imageCesium,
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

ExtensionImageCesiumUnreal::ExtensionImageCesiumUnreal(
    const CesiumAsync::SharedFuture<void>& future)
    : _pTextureResource(nullptr), _futureCreateResource(future) {}

const TSharedPtr<FCesiumTextureResource>&
ExtensionImageCesiumUnreal::getTextureResource() const {
  return this->_pTextureResource;
}

CesiumAsync::SharedFuture<void>& ExtensionImageCesiumUnreal::getFuture() {
  return this->_futureCreateResource;
}

const CesiumAsync::SharedFuture<void>&
ExtensionImageCesiumUnreal::getFuture() const {
  return this->_futureCreateResource;
}

namespace {

// Returns a Future that will resolve when the image is loaded. It _may_ also
// return a Promise, in which case the calling thread is responsible for doing
// the loading and should resolve the Promise when it's done.
std::pair<ExtensionImageCesiumUnreal&, std::optional<Promise<void>>>
getOrCreateImageFuture(
    const AsyncSystem& asyncSystem,
    ImageCesium& imageCesium) {
  std::scoped_lock lock(createExtensionMutex);

  ExtensionImageCesiumUnreal* pExtension =
      imageCesium.getExtension<ExtensionImageCesiumUnreal>();
  if (!pExtension) {
    // This thread will work on this image.
    Promise<void> promise = asyncSystem.createPromise<void>();
    ExtensionImageCesiumUnreal& extension =
        imageCesium.addExtension<ExtensionImageCesiumUnreal>(
            promise.getFuture().share());
    return {extension, std::move(promise)};
  } else {
    // Another thread is already working on this image.
    return {*pExtension, std::nullopt};
  }
}

} // namespace
