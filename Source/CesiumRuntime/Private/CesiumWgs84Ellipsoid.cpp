// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumWgs84Ellipsoid.h"
#include "CesiumEllipsoidFunctions.h"
#include "VecMath.h"
#include <CesiumGeospatial/Ellipsoid.h>

using namespace CesiumGeospatial;
using namespace CesiumUtility;

FVector UCesiumWgs84Ellipsoid::GetRadii() {
  const glm::dvec3& radii = Ellipsoid::WGS84.getRadii();
  return VecMath::createVector(radii);
}

double UCesiumWgs84Ellipsoid::GetMaximumRadius() {
  return Ellipsoid::WGS84.getRadii().x;
}

double UCesiumWgs84Ellipsoid::GetMinimumRadius() {
  return Ellipsoid::WGS84.getRadii().z;
}

FVector UCesiumWgs84Ellipsoid::ScaleToGeodeticSurface(
    const FVector& EarthCenteredEarthFixedPosition) {
  return CesiumEllipsoidFunctions::ScaleToGeodeticSurface(
      Ellipsoid::WGS84,
      EarthCenteredEarthFixedPosition);
}

FVector UCesiumWgs84Ellipsoid::GeodeticSurfaceNormal(
    const FVector& EarthCenteredEarthFixedPosition) {
  return CesiumEllipsoidFunctions::GeodeticSurfaceNormal(
      Ellipsoid::WGS84,
      EarthCenteredEarthFixedPosition);
}

FVector UCesiumWgs84Ellipsoid::LongitudeLatitudeHeightToEarthCenteredEarthFixed(
    const FVector& LongitudeLatitudeHeight) {
  return CesiumEllipsoidFunctions::
      LongitudeLatitudeHeightToEllipsoidCenteredEllipsoidFixed(
          Ellipsoid::WGS84,
          LongitudeLatitudeHeight);
}

FVector UCesiumWgs84Ellipsoid::EarthCenteredEarthFixedToLongitudeLatitudeHeight(
    const FVector& EarthCenteredEarthFixedPosition) {
  return CesiumEllipsoidFunctions::
      EllipsoidCenteredEllipsoidFixedToLongitudeLatitudeHeight(
          Ellipsoid::WGS84,
          EarthCenteredEarthFixedPosition);
}

FMatrix UCesiumWgs84Ellipsoid::EastNorthUpToEarthCenteredEarthFixed(
    const FVector& EarthCenteredEarthFixedPosition) {
  return CesiumEllipsoidFunctions::EastNorthUpToEllipsoidCenteredEllipsoidFixed(
      Ellipsoid::WGS84,
      EarthCenteredEarthFixedPosition);
}
