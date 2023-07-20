// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumWgs84Ellipsoid.h"
#include "VecMath.h"
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumUtility/Math.h>

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
    const FVector& earthCenteredEarthFixed) {
  std::optional<glm::dvec3> result = Ellipsoid::WGS84.scaleToGeodeticSurface(
      VecMath::createVector3D(earthCenteredEarthFixed));
  if (result) {
    return VecMath::createVector(*result);
  } else {
    return FVector(0.0, 0.0, 0.0);
  }
}

FVector UCesiumWgs84Ellipsoid::GeodeticSurfaceNormal(
    const FVector& earthCenteredEarthFixed) {
  return VecMath::createVector(Ellipsoid::WGS84.geodeticSurfaceNormal(
      VecMath::createVector3D(earthCenteredEarthFixed)));
}

FVector UCesiumWgs84Ellipsoid::LongitudeLatitudeHeightToEarthCenteredEarthFixed(
    const FVector& longitudeLatitudeHeight) {
  glm::dvec3 cartesian =
      Ellipsoid::WGS84.cartographicToCartesian(Cartographic::fromDegrees(
          longitudeLatitudeHeight.X,
          longitudeLatitudeHeight.Y,
          longitudeLatitudeHeight.Z));
  return VecMath::createVector(cartesian);
}

FVector UCesiumWgs84Ellipsoid::EarthCenteredEarthFixedToLongitudeLatitudeHeight(
    const FVector& earthCenteredEarthFixed) {
  std::optional<Cartographic> result = Ellipsoid::WGS84.cartesianToCartographic(
      VecMath::createVector3D(earthCenteredEarthFixed));
  if (result) {
    return FVector(
        Math::radiansToDegrees(result->longitude),
        Math::radiansToDegrees(result->latitude),
        result->height);
  } else {
    return FVector(0.0, 0.0, 0.0);
  }
}
