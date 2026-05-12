// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumRuntime.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Misc/Optional.h"
#include "CesiumWgs84Ellipsoid.generated.h"

UCLASS()
class CESIUMRUNTIME_API UCesiumWgs84Ellipsoid
    : public UBlueprintFunctionLibrary {
  GENERATED_BODY()

public:
  /**
   * Gets the radii of the WGS84 ellipsoid in its x-, y-, and z-directions in
   * meters.
   */
  UFUNCTION(BlueprintPure, Category = "Cesium|Ellipsoid|WGS84")
  static FVector GetRadii();

  /**
   * Gets the maximum radius of the WGS84 ellipsoid in any dimension, in meters.
   */
  UFUNCTION(BlueprintPure, Category = "Cesium|Ellipsoid|WGS84")
  static double GetMaximumRadius();

  /**
   * Gets the minimum radius of the WGS854 ellipsoid in any dimension, in
   * meters.
   */
  UFUNCTION(BlueprintPure, Category = "Cesium|Ellipsoid|WGS84")
  static double GetMinimumRadius();

  /**
   * Scale the given Earth-Centered, Earth-Fixed position along the geodetic
   * surface normal so that it is on the surface of the ellipsoid. If the
   * position is near the center of the ellipsoid, the result will have the
   * value (0,0,0) because the surface position is undefined.
   */
  UFUNCTION(
      BlueprintPure,
      Category = "Cesium|Ellipsoid|WGS84",
      meta = (ReturnDisplayName = "SurfacePosition"))
  static FVector
  ScaleToGeodeticSurface(const FVector& EarthCenteredEarthFixedPosition);

  /**
   * Computes the normal of the plane tangent to the surface of the ellipsoid
   * at the provided Earth-Centered, Earth-Fixed position.
   */
  UFUNCTION(
      BlueprintPure,
      Category = "Cesium|Ellipsoid|WGS84",
      meta = (ReturnDisplayName = "SurfaceNormalVector"))
  static FVector
  GeodeticSurfaceNormal(const FVector& EarthCenteredEarthFixedPosition);

  /**
   * Convert longitude in degrees (X), latitude in degrees (Y), and height above
   * the WGS84 ellipsoid in meters (Z) to Earth-Centered, Earth-Fixed (ECEF)
   * coordinates.
   */
  UFUNCTION(
      BlueprintPure,
      Category = "Cesium|Ellipsoid|WGS84",
      meta = (ReturnDisplayName = "EarthCenteredEarthFixedPosition"))
  static FVector LongitudeLatitudeHeightToEarthCenteredEarthFixed(
      const FVector& LongitudeLatitudeHeight);

  /**
   * Convert Earth-Centered, Earth-Fixed (ECEF) coordinates to longitude in
   * degrees (X), latitude in degrees (Y), and height above the WGS84 ellipsoid
   * in meters (Z). If the position is near the center of the Earth, the result
   * will have the value (0,0,0) because the longitude, latitude, and height are
   * undefined.
   */
  UFUNCTION(
      BlueprintPure,
      Category = "Cesium|Ellipsoid|WGS84",
      meta = (ReturnDisplayName = "LongitudeLatitudeHeight"))
  static FVector EarthCenteredEarthFixedToLongitudeLatitudeHeight(
      const FVector& EarthCenteredEarthFixedPosition);

  /**
   * Computes the transformation matrix from the local East-North-Up (ENU) frame
   * to Earth-Centered, Earth-Fixed (ECEF) at the specified ECEF location.
   */
  static FMatrix EastNorthUpToEarthCenteredEarthFixed(
      const FVector& EarthCenteredEarthFixedPosition);
};
