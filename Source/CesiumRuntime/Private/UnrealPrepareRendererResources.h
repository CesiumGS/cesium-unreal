#pragma once

#include <Cesium3DTilesSelection/IPrepareRendererResources.h>

class ACesium3DTileset;

/**
 * An implementation of Cesium Native's IPrepareRendererResources that creates
 * Unreal objects for 3D Tiles tiles and raster overlays.
 */
class UnrealPrepareRendererResources
    : public Cesium3DTilesSelection::IPrepareRendererResources {
public:
  UnrealPrepareRendererResources(ACesium3DTileset* pActor);

  virtual CesiumAsync::Future<
      Cesium3DTilesSelection::TileLoadResultAndRenderResources>
  prepareInLoadThread(
      const CesiumAsync::AsyncSystem& asyncSystem,
      Cesium3DTilesSelection::TileLoadResult&& tileLoadResult,
      const glm::dmat4& transform,
      const std::any& rendererOptions) override;

  virtual void* prepareInMainThread(
      Cesium3DTilesSelection::Tile& tile,
      void* pLoadThreadResult) override;

  virtual void free(
      Cesium3DTilesSelection::Tile& tile,
      void* pLoadThreadResult,
      void* pMainThreadResult) noexcept override;

  virtual void* prepareRasterInLoadThread(
      CesiumGltf::ImageAsset& image,
      const std::any& rendererOptions) override;

  virtual void* prepareRasterInMainThread(
      CesiumRasterOverlays::RasterOverlayTile& rasterTile,
      void* pLoadThreadResult) override;

  virtual void freeRaster(
      const CesiumRasterOverlays::RasterOverlayTile& rasterTile,
      void* pLoadThreadResult,
      void* pMainThreadResult) noexcept override;

  virtual void attachRasterInMainThread(
      const Cesium3DTilesSelection::Tile& tile,
      int32_t overlayTextureCoordinateID,
      const CesiumRasterOverlays::RasterOverlayTile& rasterTile,
      void* pMainThreadRendererResources,
      const glm::dvec2& translation,
      const glm::dvec2& scale) override;

  virtual void detachRasterInMainThread(
      const Cesium3DTilesSelection::Tile& tile,
      int32_t overlayTextureCoordinateID,
      const CesiumRasterOverlays::RasterOverlayTile& rasterTile,
      void* pMainThreadRendererResources) noexcept override;

private:
  ACesium3DTileset* _pActor;
};
