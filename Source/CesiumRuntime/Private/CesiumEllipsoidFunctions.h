// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#pragma once

#include "Math/Matrix.h"

namespace CesiumGeospatial {
class Ellipsoid;
};

/**
 * A collection of methods for working with {@link CesiumGeospatial::Ellipsoid}
 * objects from Unreal.
 */
class CesiumEllipsoidFunctions {
public:
  /**
   * Scale the given Ellipsoid-Centered, Ellipsoid-Fixed position along the
   * geodetic surface normal so that it is on the surface of the ellipsoid. If
   * the position is near the center of the ellipsoid, the result will have the
   * value (0,0,0) because the surface position is undefined.
   */
  static FVector ScaleToGeodeticSurface(
      const CesiumGeospatial::Ellipsoid& Ellipsoid,
      const FVector& EllipsoidCenteredEllipsoidFixedPosition);

  /**
   * Computes the normal of the plane tangent to the surface of the ellipsoid
   * at the provided Ellipsoid-Centered, Ellipsoid-Fixed position.
   */
  static FVector GeodeticSurfaceNormal(
      const CesiumGeospatial::Ellipsoid& Ellipsoid,
      const FVector& EarthCenteredEarthFixedPosition);

  /**
   * Convert longitude in degrees (X), latitude in degrees (Y), and height above
   * the ellipsoid in meters (Z) to Ellipsoid-Centered, Ellipsoid-Fixed (ECEF)
   * coordinates.
   */
  static FVector LongitudeLatitudeHeightToEllipsoidCenteredEllipsoidFixed(
      const CesiumGeospatial::Ellipsoid& Ellipsoid,
      const FVector& LongitudeLatitudeHeight);

  /**
   * Convert Ellipsoid-Centered, Ellipsoid-Fixed (ECEF) coordinates to longitude
   * in degrees (X), latitude in degrees (Y), and height above the ellipsoid in
   * meters (Z). If the position is near the center of the Earth, the result
   * will have the value (0,0,0) because the longitude, latitude, and height are
   * undefined.
   */
  static FVector EllipsoidCenteredEllipsoidFixedToLongitudeLatitudeHeight(
      const CesiumGeospatial::Ellipsoid& Ellipsoid,
      const FVector& EllipsoidCenteredEllipsoidFixedPosition);

  /**
   * Computes the transformation matrix from the local East-North-Up (ENU) frame
   * to Ellipsoid-Centered, Ellipsoid-Fixed (ECEF) at the specified ECEF
   * location.
   */
  static FMatrix EastNorthUpToEllipsoidCenteredEllipsoidFixed(
      const CesiumGeospatial::Ellipsoid& Ellipsoid,
      const FVector& EllipsoidCenteredEllipsoidFixedPosition);
};
