// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGeoreference.h"
#include "CesiumGeospatial/CartographicPolygon.h"
#include "CesiumGeospatial/GlobeRectangle.h"
#include "CesiumGlobeAnchorComponent.h"
#include "Components/SplineComponent.h"
#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include <vector>

#include "CesiumCartographicPolygon.generated.h"

/**
 * A spline-based polygon actor used to rasterize 2D polygons on top of
 * Cesium 3D Tileset actors.
 */
UCLASS(ClassGroup = (Cesium), meta = (BlueprintSpawnableComponent))
class CESIUMRUNTIME_API ACesiumCartographicPolygon : public AActor {

  GENERATED_BODY()

public:
  ACesiumCartographicPolygon();

  /**
   * The polygon.
   */
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cesium")
  USplineComponent* Polygon;

  /**
   * The Globe Anchor Component that precisely ties this Polygon to the Globe.
   */
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cesium")
  UCesiumGlobeAnchorComponent* GlobeAnchor;

  virtual void OnConstruction(const FTransform& Transform) override;

  /**
   * Creates and returns a CartographicPolygon object
   * created from the current spline selection.
   *
   * @param worldToTileset The transformation from Unreal world coordinates to
   * the coordinates of the Cesium3DTileset Actor for which the cartographic
   * polygon is being created.
   */
  CesiumGeospatial::CartographicPolygon
  CreateCartographicPolygon(const FTransform& worldToTileset) const;

  // AActor overrides
  virtual void PostLoad() override;

protected:
  virtual void BeginPlay() override;

private:
  void MakeLinear();
};
