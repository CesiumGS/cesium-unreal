#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include <Cesium3DTilesSelection/Tile.h>
#include "CesiumTile.generated.h"

/**
 * A UObject representation of a Cesium Tile.
 * This class provides an interface for accessing properties of a Cesium Tile
 * from within Unreal Engine. It exposes the Bounds property, which can be
 * accessed from Blueprints, and provides a helper function for testing
 * intersection with a primitive component.
 */
UCLASS()
class CESIUMRUNTIME_API UCesiumTile : public UObject {
  GENERATED_BODY()

  // The underlying Cesium Tile
  const Cesium3DTilesSelection::Tile* _pTile;

  // The transform of the tile
  glm::dmat4 _transform;

public:
  /**
   * Returns the bounds of the tile in Unreal Absolute World coordinates.
   * This function returns the bounding box and bounding sphere that enclose the
   * tile.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  FBoxSphereBounds GetBounds() const;

  /**
   * Tests whether a primitive component intersects with this tile.
   * This function provides a convenient way to test for intersection between a
   * primitive component and this tile.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  bool IsIntersecting(UPrimitiveComponent* Other) const;

  friend class CesiumTileExcluderAdapter;
};
