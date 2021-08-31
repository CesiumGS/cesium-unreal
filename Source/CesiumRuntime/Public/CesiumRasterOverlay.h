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
  UPROPERTY(EditAnywhere, Category = "Cesium")
  FString MaterialLayerKey = "Overlay0";

  // Sets default values for this component's properties
  UCesiumRasterOverlay();

  /**
   * Adds this raster overlay to its owning Cesium 3D Tiles Actor. If the
   * overlay is already added or if this component's Owner is not a Cesium 3D
   * Tileset, this method does nothing.
   */
  void AddToTileset();

  /**
   * Removes this raster overlay from its owning Cesium 3D Tiles Actor. If the
   * overlay is not yet added or if this component's Owner is not a Cesium 3D
   * Tileset, this method does nothing.
   */
  void RemoveFromTileset();

  virtual void Activate(bool bReset) override;
  virtual void Deactivate() override;
  virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

protected:
#if WITH_EDITOR
  // Called when properties are changed in the editor
  virtual void
  PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

  Cesium3DTilesSelection::Tileset* FindTileset() const;
  virtual std::unique_ptr<Cesium3DTilesSelection::RasterOverlay> CreateOverlay()
      PURE_VIRTUAL(UCesiumRasterOverlay::CreateOverlay, return nullptr;);

private:
  Cesium3DTilesSelection::RasterOverlay* _pOverlay;
};
