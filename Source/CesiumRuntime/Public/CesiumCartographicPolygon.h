// Copyright 2020-2021 CesiumGS, Inc. and Contributors

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

  UPROPERTY()
  ACesiumGeoreference* Georeference;

  UPROPERTY()
  USplineComponent* Polygon;

  UPROPERTY()
  UCesiumGlobeAnchorComponent* GlobeAnchor;

  virtual void OnConstruction(const FTransform& Transform) override;

  /**
   * Creates and returns a CartographicPolygon object
   * created from the current spline selection.
   */
  CesiumGeospatial::CartographicPolygon CreateCartographicPolygon() const;

protected:
  virtual void BeginPlay() override;

private:
  void MakeLinear();
};
