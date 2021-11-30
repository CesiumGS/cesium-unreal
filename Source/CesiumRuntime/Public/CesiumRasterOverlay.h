// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "Cesium3DTilesSelection/RasterOverlay.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include <memory>
#include "CesiumRasterOverlay.generated.h"

namespace Cesium3DTilesSelection {
class Tileset;
}

/**
 * A quadtree pyramid of 2D raster images meant to be draped over a Cesium 3D
 * Tileset. Raster overlays are commonly used for satellite imagery, street
 * maps, and more.
 */
UCLASS(Abstract)
class CESIUMRUNTIME_API UCesiumRasterOverlay : public UActorComponent {
  GENERATED_BODY()

public:
  /**
   * The key to use to match this overlay to a material layer.
   *
   * When using Material Layers, any material layers inside a "Cesium" layer
   * stack with a name that matches this name will have their Texture,
   * TranslationScale, and TextureCoordinateIndex properties set automatically
   * so that a ML_CesiumOverlay layer function (or similar) will correctly
   * sample from this overlay.
   */
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cesium")
  FString MaterialLayerKey = "Overlay0";

  // Sets default values for this component's properties
  UCesiumRasterOverlay();

  /**
   * Adds this raster overlay to its owning Cesium 3D Tiles Actor. If the
   * overlay is already added or if this component's Owner is not a Cesium 3D
   * Tileset, this method does nothing.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void AddToTileset();

  /**
   * Removes this raster overlay from its owning Cesium 3D Tiles Actor. If the
   * overlay is not yet added or if this component's Owner is not a Cesium 3D
   * Tileset, this method does nothing.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void RemoveFromTileset();

  /**
   * Refreshes this tileset by removing and re-adding it.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void Refresh();

  UFUNCTION(BlueprintCallable, Category = "Cesium")
  float GetMaximumScreenSpaceError() const;

  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void SetMaximumScreenSpaceError(float Value);

  UFUNCTION(BlueprintCallable, Category = "Cesium")
  int32 GetMaximumTextureSize() const;

  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void SetMaximumTextureSize(int32 Value);

  UFUNCTION(BlueprintCallable, Category = "Cesium")
  int32 GetMaximumSimultaneousTileLoads() const;

  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void SetMaximumSimultaneousTileLoads(int32 Value);

  UFUNCTION(BlueprintCallable, Category = "Cesium")
  int64 GetSubTileCacheBytes() const;

  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void SetSubTileCacheBytes(int64 Value);

  virtual void Activate(bool bReset) override;
  virtual void Deactivate() override;
  virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

protected:
  /**
   * The maximum number of pixels of error when rendering this overlay.
   * This is used to select an appropriate level-of-detail.
   *
   * When this property has its default value, 2.0, it means that raster overlay
   * images will be sized so that, when zoomed in closest, a single pixel in
   * the raster overlay maps to approximately 2x2 pixels on the screen.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      BlueprintGetter = GetMaximumScreenSpaceError,
      BlueprintSetter = SetMaximumScreenSpaceError,
      Category = "Cesium")
  float MaximumScreenSpaceError = 2.0;

  /**
   * The maximum texel size of raster overlay textures, in either
   * direction.
   *
   * Images created by this overlay will be no more than this number of texels
   * in either direction. This may result in reduced raster overlay detail in
   * some cases.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      BlueprintGetter = GetMaximumTextureSize,
      BlueprintSetter = SetMaximumTextureSize,
      Category = "Cesium")
  int32 MaximumTextureSize = 2048;

  /**
   * The maximum number of overlay tiles that may simultaneously be in
   * the process of loading.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      BlueprintGetter = GetMaximumSimultaneousTileLoads,
      BlueprintSetter = SetMaximumSimultaneousTileLoads,
      Category = "Cesium")
  int32 MaximumSimultaneousTileLoads = 20;

  /**
   * The maximum number of bytes to use to cache sub-tiles in memory.
   *
   * This is used by provider types, that have an underlying tiling
   * scheme that may not align with the tiling scheme of the geometry tiles on
   * which the raster overlay tiles are draped. Because a single sub-tile may
   * overlap multiple geometry tiles, it is useful to cache loaded sub-tiles
   * in memory in case they're needed again soon. This property controls the
   * maximum size of that cache.
   */
  UPROPERTY(
      EditAnywhere,
      BlueprintReadWrite,
      BlueprintGetter = GetSubTileCacheBytes,
      BlueprintSetter = SetSubTileCacheBytes,
      Category = "Cesium")
  int64 SubTileCacheBytes = 16 * 1024 * 1024;

#if WITH_EDITOR
  // Called when properties are changed in the editor
  virtual void
  PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

  Cesium3DTilesSelection::Tileset* FindTileset() const;

  virtual std::unique_ptr<Cesium3DTilesSelection::RasterOverlay> CreateOverlay(
      const Cesium3DTilesSelection::RasterOverlayOptions& options = {})
      PURE_VIRTUAL(UCesiumRasterOverlay::CreateOverlay, return nullptr;);

  virtual void OnAdd(
      Cesium3DTilesSelection::Tileset* pTileset,
      Cesium3DTilesSelection::RasterOverlay* pOverlay) {}
  virtual void OnRemove(
      Cesium3DTilesSelection::Tileset* pTileset,
      Cesium3DTilesSelection::RasterOverlay* pOverlay) {}

private:
  Cesium3DTilesSelection::RasterOverlay* _pOverlay;
};
