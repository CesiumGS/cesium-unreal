// Copyright 2020-2025 CesiumGS, Inc. and Contributors

#include "UnrealPrepareRendererResources.h"
#include "Cesium3DTileset.h"
#include "Cesium3DTilesetLifecycleEventReceiver.h"
#include "CesiumGltfComponent.h"
#include "CesiumLifetime.h"
#include "CesiumRasterOverlay.h"
#include "CesiumRuntime.h"
#include "CesiumVoxelRendererComponent.h"
#include "CreateGltfOptions.h"
#include "ExtensionImageAssetUnreal.h"

#include <Cesium3DTilesSelection/Tile.h>
#include <Cesium3DTilesSelection/TileLoadResult.h>
#include <CesiumAsync/AsyncSystem.h>
#include <CesiumGeospatial/Ellipsoid.h>
#include <glm/mat4x4.hpp>

UnrealPrepareRendererResources::UnrealPrepareRendererResources(
    ACesium3DTileset* pActor)
    : _pActor(pActor) {}

CesiumAsync::Future<Cesium3DTilesSelection::TileLoadResultAndRenderResources>
UnrealPrepareRendererResources::prepareInLoadThread(
    const CesiumAsync::AsyncSystem& asyncSystem,
    Cesium3DTilesSelection::TileLoadResult&& tileLoadResult,
    const glm::dmat4& transform,
    const std::any& rendererOptions) {
  CreateGltfOptions::CreateModelOptions options(std::move(tileLoadResult));
  if (!options.pModel) {
    return asyncSystem.createResolvedFuture(
        Cesium3DTilesSelection::TileLoadResultAndRenderResources{
            std::move(options.tileLoadResult),
            nullptr});
  }

  options.alwaysIncludeTangents = this->_pActor->GetAlwaysIncludeTangents();
  options.createPhysicsMeshes = this->_pActor->GetCreatePhysicsMeshes();

  options.ignoreKhrMaterialsUnlit = this->_pActor->GetIgnoreKhrMaterialsUnlit();

  options.pFeaturesMetadata = this->_pActor->_pFeaturesMetadataComponent;

  PRAGMA_DISABLE_DEPRECATION_WARNINGS
  if (this->_pActor->_metadataDescription_DEPRECATED) {
    options.pEncodedMetadataDescription_DEPRECATED =
        &(*this->_pActor->_metadataDescription_DEPRECATED);
  }
  PRAGMA_ENABLE_DEPRECATION_WARNINGS

  if (this->_pActor->_pVoxelRendererComponent) {
    options.pVoxelOptions = &this->_pActor->_pVoxelRendererComponent->Options;
  }

  const CesiumGeospatial::Ellipsoid& ellipsoid = tileLoadResult.ellipsoid;

  CesiumAsync::Future<UCesiumGltfComponent::CreateOffGameThreadResult>
      pHalfFuture = UCesiumGltfComponent::CreateOffGameThread(
          asyncSystem,
          transform,
          std::move(options),
          ellipsoid);

  return MoveTemp(pHalfFuture)
      .thenImmediately(
          [](UCesiumGltfComponent::CreateOffGameThreadResult&& result)
              -> Cesium3DTilesSelection::TileLoadResultAndRenderResources {
            return Cesium3DTilesSelection::TileLoadResultAndRenderResources{
                std::move(result.TileLoadResult),
                result.HalfConstructed.Release()};
          });
}

void* UnrealPrepareRendererResources::prepareInMainThread(
    Cesium3DTilesSelection::Tile& tile,
    void* pLoadThreadResult) {
  Cesium3DTilesSelection::TileContent& content = tile.getContent();
  if (content.isRenderContent()) {
    TUniquePtr<UCesiumGltfComponent::HalfConstructed> pHalf(
        reinterpret_cast<UCesiumGltfComponent::HalfConstructed*>(
            pLoadThreadResult));
    Cesium3DTilesSelection::TileRenderContent& renderContent =
        *content.getRenderContent();
    return UCesiumGltfComponent::CreateOnGameThread(
        renderContent.getModel(),
        this->_pActor,
        std::move(pHalf),
        _pActor->GetCesiumTilesetToUnrealRelativeWorldTransform(),
        this->_pActor->GetMaterial(),
        this->_pActor->GetTranslucentMaterial(),
        this->_pActor->GetWaterMaterial(),
        this->_pActor->GetCustomDepthParameters(),
        tile,
        this->_pActor->GetCreateNavCollision(),
        this->_pActor->GetEnableDoubleSidedCollisions());
  }
  // UE_LOG(LogCesium, VeryVerbose, TEXT("No content for tile"));
  return nullptr;
}

void UnrealPrepareRendererResources::free(
    Cesium3DTilesSelection::Tile& tile,
    void* pLoadThreadResult,
    void* pMainThreadResult) noexcept {
  if (pLoadThreadResult) {
    UCesiumGltfComponent::HalfConstructed* pHalf =
        reinterpret_cast<UCesiumGltfComponent::HalfConstructed*>(
            pLoadThreadResult);
    delete pHalf;
  } else if (pMainThreadResult) {
    UCesiumGltfComponent* pGltf =
        reinterpret_cast<UCesiumGltfComponent*>(pMainThreadResult);
    if (ICesium3DTilesetLifecycleEventReceiver* Receiver =
            this->_pActor->GetLifecycleEventReceiver()) {
      Receiver->OnTileUnloading(*pGltf);
    }
    CesiumLifetime::destroyComponentRecursively(pGltf);
  }
}

void* UnrealPrepareRendererResources::prepareRasterInLoadThread(
    CesiumGltf::ImageAsset& image,
    const std::any& rendererOptions) {
  auto ppOptions =
      std::any_cast<FRasterOverlayRendererOptions*>(&rendererOptions);
  check(ppOptions != nullptr && *ppOptions != nullptr);
  if (ppOptions == nullptr || *ppOptions == nullptr) {
    return nullptr;
  }

  auto pOptions = *ppOptions;

  if (pOptions->useMipmaps) {
    std::optional<std::string> errorMessage =
        CesiumGltfReader::ImageDecoder::generateMipMaps(image);
    if (errorMessage) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT("%s"),
          UTF8_TO_TCHAR(errorMessage->c_str()));
    }
  }

  // TODO: sRGB should probably be configurable on the raster overlay.
  bool sRGB = true;

  const ExtensionImageAssetUnreal& extension =
      ExtensionImageAssetUnreal::getOrCreate(
          CesiumAsync::AsyncSystem(nullptr), // TODO
          image,
          sRGB,
          pOptions->useMipmaps,
          std::nullopt);

  // Because raster overlay images are never shared (at least currently!), the
  // future should already be resolved by the time we get here.
  check(extension.getFuture().isReady());

  auto texture = CesiumTextureUtility::loadTextureAnyThreadPart(
      image,
      TextureAddress::TA_Clamp,
      TextureAddress::TA_Clamp,
      pOptions->filter,
      pOptions->useMipmaps,
      pOptions->group,
      sRGB,
      std::nullopt);

  return texture.Release();
}

void* UnrealPrepareRendererResources::prepareRasterInMainThread(
    CesiumRasterOverlays::RasterOverlayTile& rasterTile,
    void* pLoadThreadResult) {
  TUniquePtr<CesiumTextureUtility::LoadedTextureResult> pLoadedTexture{
      static_cast<CesiumTextureUtility::LoadedTextureResult*>(
          pLoadThreadResult)};

  if (!pLoadedTexture) {
    return nullptr;
  }

  CesiumUtility::IntrusivePointer<
      CesiumTextureUtility::ReferenceCountedUnrealTexture>
      pTexture =
          CesiumTextureUtility::loadTextureGameThreadPart(pLoadedTexture.Get());
  if (!pTexture) {
    return nullptr;
  }

  // Don't let this ReferenceCountedUnrealTexture be destroyed when the
  // intrusive pointer goes out of scope.
  pTexture->addReference();
  return pTexture.get();
}

void UnrealPrepareRendererResources::freeRaster(
    const CesiumRasterOverlays::RasterOverlayTile& rasterTile,
    void* pLoadThreadResult,
    void* pMainThreadResult) noexcept {
  if (pLoadThreadResult) {
    CesiumTextureUtility::LoadedTextureResult* pLoadedTexture =
        static_cast<CesiumTextureUtility::LoadedTextureResult*>(
            pLoadThreadResult);
    delete pLoadedTexture;
  }

  if (pMainThreadResult) {
    CesiumTextureUtility::ReferenceCountedUnrealTexture* pTexture =
        static_cast<CesiumTextureUtility::ReferenceCountedUnrealTexture*>(
            pMainThreadResult);
    pTexture->releaseReference();
  }
}

void UnrealPrepareRendererResources::attachRasterInMainThread(
    const Cesium3DTilesSelection::Tile& tile,
    int32_t overlayTextureCoordinateID,
    const CesiumRasterOverlays::RasterOverlayTile& rasterTile,
    void* pMainThreadRendererResources,
    const glm::dvec2& translation,
    const glm::dvec2& scale) {
  const Cesium3DTilesSelection::TileContent& content = tile.getContent();
  const Cesium3DTilesSelection::TileRenderContent* pRenderContent =
      content.getRenderContent();
  if (pMainThreadRendererResources != nullptr && pRenderContent != nullptr) {
    UCesiumGltfComponent* pGltfContent =
        reinterpret_cast<UCesiumGltfComponent*>(
            pRenderContent->getRenderResources());
    if (pGltfContent) {
      pGltfContent->AttachRasterTile(
          tile,
          rasterTile,
          static_cast<CesiumTextureUtility::ReferenceCountedUnrealTexture*>(
              pMainThreadRendererResources)
              ->getUnrealTexture(),
          translation,
          scale,
          overlayTextureCoordinateID);
    }
  }
}

void UnrealPrepareRendererResources::detachRasterInMainThread(
    const Cesium3DTilesSelection::Tile& tile,
    int32_t overlayTextureCoordinateID,
    const CesiumRasterOverlays::RasterOverlayTile& rasterTile,
    void* pMainThreadRendererResources) noexcept {
  const Cesium3DTilesSelection::TileContent& content = tile.getContent();
  const Cesium3DTilesSelection::TileRenderContent* pRenderContent =
      content.getRenderContent();
  if (pRenderContent) {
    UCesiumGltfComponent* pGltfContent =
        reinterpret_cast<UCesiumGltfComponent*>(
            pRenderContent->getRenderResources());
    if (pMainThreadRendererResources != nullptr && pGltfContent != nullptr) {
      pGltfContent->DetachRasterTile(
          tile,
          rasterTile,
          static_cast<CesiumTextureUtility::ReferenceCountedUnrealTexture*>(
              pMainThreadRendererResources)
              ->getUnrealTexture());
    }
  }
}
