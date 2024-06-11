// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "Engine/DataAsset.h"
#include <CesiumGeospatial/LocalHorizontalCoordinateSystem.h>
#include "CesiumEllipsoid.generated.h"

namespace CesiumGeospatial {
class Ellipsoid;
};

UCLASS()
class CESIUMRUNTIME_API UCesiumEllipsoid : public UDataAsset {
  GENERATED_BODY()

public:
  /**
   * Gets the radii of the ellipsoid in its x-, y-, and z-directions in
   * meters.
   */
  UFUNCTION(BlueprintPure, Category = "Cesium|Ellipsoid")
  FVector GetRadii();

  /**
   * Gets the maximum radius of the ellipsoid in any dimension, in meters.
   */
  UFUNCTION(BlueprintPure, Category = "Cesium|Ellipsoid")
  double GetMaximumRadius();

  /**
   * Gets the minimum radius of the ellipsoid in any dimension, in
   * meters.
   */
  UFUNCTION(BlueprintPure, Category = "Cesium|Ellipsoid")
  double GetMinimumRadius();

  /**
   * Scale the given Ellipsoid-Centered, Ellipsoid-Fixed position along the
   * geodetic surface normal so that it is on the surface of the ellipsoid. If
   * the position is near the center of the ellipsoid, the result will have the
   * value (0,0,0) because the surface position is undefined.
   */
  UFUNCTION(
      BlueprintPure,
      Category = "Cesium|Ellipsoid",
      meta = (ReturnDisplayName = "SurfacePosition"))
  FVector
  ScaleToGeodeticSurface(const FVector& EarthCenteredEarthFixedPosition);

  /**
   * Computes the normal of the plane tangent to the surface of the ellipsoid
   * at the provided Ellipsoid-Centered, Ellipsoid-Fixed position.
   */
  UFUNCTION(
      BlueprintPure,
      Category = "Cesium|Ellipsoid",
      meta = (ReturnDisplayName = "SurfaceNormalVector"))
  FVector GeodeticSurfaceNormal(const FVector& EarthCenteredEarthFixedPosition);

  /**
   * Convert longitude in degrees (X), latitude in degrees (Y), and height above
   * the ellipsoid in meters (Z) to Ellipsoid-Centered, Ellipsoid-Fixed (ECEF)
   * coordinates.
   */
  UFUNCTION(
      BlueprintPure,
      Category = "Cesium|Ellipsoid",
      meta = (ReturnDisplayName = "EarthCenteredEarthFixedPosition"))
  FVector LongitudeLatitudeHeightToEllipsoidCenteredEllipsoidFixed(
      const FVector& LongitudeLatitudeHeight);

  /**
   * Convert Ellipsoid-Centered, Ellipsoid-Fixed (ECEF) coordinates to longitude
   * in degrees (X), latitude in degrees (Y), and height above the ellipsoid in
   * meters (Z). If the position is near the center of the Ellipsoid, the result
   * will have the value (0,0,0) because the longitude, latitude, and height are
   * undefined.
   */
  UFUNCTION(
      BlueprintPure,
      Category = "Cesium|Ellipsoid",
      meta = (ReturnDisplayName = "LongitudeLatitudeHeight"))
  FVector EllipsoidCenteredEllipsoidFixedToLongitudeLatitudeHeight(
      const FVector& EarthCenteredEarthFixedPosition);

  /**
   * Computes the transformation matrix from the local East-North-Up (ENU) frame
   * to Ellipsoid-Centered, Ellipsoid-Fixed (ECEF) at the specified ECEF
   * location.
   */
  FMatrix EastNorthUpToEllipsoidCenteredEllipsoidFixed(
      const FVector& EarthCenteredEarthFixedPosition);

  /**
   * Returns a new {@link CesiumGeospatial::LocalHorizontalCoordinateSystem}
   * with the given scale, center, and ellipsoid.
   */
  CesiumGeospatial::LocalHorizontalCoordinateSystem
  CreateCoordinateSystem(const FVector& Center, double Scale);

  /**
   * Returns the underlying {@link CesiumGeospatial::Ellipsoid}
   */
  TSharedPtr<CesiumGeospatial::Ellipsoid> GetNativeEllipsoid();

protected:
  /**
   * The radii of this ellipsoid.
   *
   * The X coordinate of the vector should be the radius of the largest axis and
   * the Z coordinate should be the radius of the smallest axis.
   */
  UPROPERTY(
      EditAnywhere,
      Category = "Cesium|Ellipsoid",
      meta = (DisplayName = "Radii"))
  FVector Radii;

private:
  TSharedPtr<CesiumGeospatial::Ellipsoid> NativeEllipsoid = nullptr;
};
