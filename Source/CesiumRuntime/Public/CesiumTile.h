// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include <Cesium3DTilesSelection/BoundingVolume.h>
#include "CesiumTile.generated.h"

/**
 * A UObject representation of a Cesium Tile.
 * This class provides an interface for accessing properties of a Cesium Tile
 * from within Unreal Engine. It exposes the Bounds property, which can be
 * accessed from Blueprints, and provides a helper function for testing
 * intersection with a primitive component.
 */
UCLASS()
class CESIUMRUNTIME_API UCesiumTile : public UPrimitiveComponent {
  GENERATED_BODY()

  glm::dmat4 _tileTransform;

  Cesium3DTilesSelection::BoundingVolume _tileBounds =
      CesiumGeometry::OrientedBoundingBox(glm::dvec3(0.0), glm::dmat3(1.0));

public:
  /**
   * Tests whether a primitive component overlaps with this tile using a sphere
   * and box comparison. This function provides a convenient way to test for
   * intersection between a primitive component and this tile.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  bool TileBoundsOverlapsPrimitive(const UPrimitiveComponent* Other) const;

  /**
   * Checks if this tile is fully inside the given primitive component using a
   * sphere and box comparison. It uses the FBox::IsInside function to compare
   * the FBox of the component and the tile's bounds.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  bool
  PrimitiveBoxFullyContainsTileBounds(const UPrimitiveComponent* Other) const;

  virtual FBoxSphereBounds
  CalcBounds(const FTransform& LocalToWorld) const override;

  friend class CesiumTileExcluderAdapter;
};
