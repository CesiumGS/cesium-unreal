// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGeospatial/CartographicPolygon.h"
#include "CesiumGlobeAnchorComponent.h"
#include "Components/SplineComponent.h"
#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include <vector>

#include "CesiumCartographicPolygon.generated.h"

UENUM(BlueprintType)
enum class ECesiumGlobeCoordinateSpace : uint8 {
  LatitudeLongitudeHeight UMETA(DisplayName = "Latitude Longitude Height"),
  EarthCenteredEarthFixed UMETA(DisplayName = "Earth Centered Earth Fixed"),
};

/**
 * A spline-based polygon actor used to rasterize 2D polygons on top of
 * Cesium 3D Tileset actors.
 */
UCLASS(ClassGroup = Cesium, meta = (BlueprintSpawnableComponent))
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

  /**
   * Sets the spline points from an array of geographic coordinates
   * @param CoordinateSpace The coordinate space used in the provided Points
   * array. Either longitude expressed in degrees (X), latitude in degrees (Y)
   * and height in meters (Z), or Earth-Centered, Earth-Fixed cartesian
   * coordinates.
   * @param Points An array of points expressed in terms of the given
   * CoordinateSpace.
   */
  UFUNCTION(BlueprintCallable, Category = "Cesium")
  void SetPolygonPoints(
      const ECesiumGlobeCoordinateSpace CoordinateSpace,
      const TArray<FVector>& Points);

  // AActor overrides
  virtual void PostLoad() override;

protected:
  virtual void BeginPlay() override;

private:
  void MakeLinear();
};
