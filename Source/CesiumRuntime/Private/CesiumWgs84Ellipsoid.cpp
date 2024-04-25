// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumWgs84Ellipsoid.h"
#include "VecMath.h"
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeTransforms.h>
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
    const FVector& EarthCenteredEarthFixedPosition) {
  std::optional<glm::dvec3> result = Ellipsoid::WGS84.scaleToGeodeticSurface(
      VecMath::createVector3D(EarthCenteredEarthFixedPosition));
  if (result) {
    return VecMath::createVector(*result);
  } else {
    return FVector(0.0, 0.0, 0.0);
  }
}

FVector UCesiumWgs84Ellipsoid::GeodeticSurfaceNormal(
    const FVector& EarthCenteredEarthFixedPosition) {
  return VecMath::createVector(Ellipsoid::WGS84.geodeticSurfaceNormal(
      VecMath::createVector3D(EarthCenteredEarthFixedPosition)));
}

FVector UCesiumWgs84Ellipsoid::LongitudeLatitudeHeightToEarthCenteredEarthFixed(
    const FVector& LongitudeLatitudeHeight) {
  glm::dvec3 cartesian =
      Ellipsoid::WGS84.cartographicToCartesian(Cartographic::fromDegrees(
          LongitudeLatitudeHeight.X,
          LongitudeLatitudeHeight.Y,
          LongitudeLatitudeHeight.Z));
  return VecMath::createVector(cartesian);
}

FVector UCesiumWgs84Ellipsoid::EarthCenteredEarthFixedToLongitudeLatitudeHeight(
    const FVector& EarthCenteredEarthFixedPosition) {
  std::optional<Cartographic> result = Ellipsoid::WGS84.cartesianToCartographic(
      VecMath::createVector3D(EarthCenteredEarthFixedPosition));
  if (result) {
    return FVector(
        Math::radiansToDegrees(result->longitude),
        Math::radiansToDegrees(result->latitude),
        result->height);
  } else {
    return FVector(0.0, 0.0, 0.0);
  }
}

FMatrix UCesiumWgs84Ellipsoid::EastNorthUpToEarthCenteredEarthFixed(
    const FVector& EarthCenteredEarthFixedPosition) {
  return VecMath::createMatrix(
      CesiumGeospatial::GlobeTransforms::eastNorthUpToFixedFrame(
          VecMath::createVector3D(EarthCenteredEarthFixedPosition),
          CesiumGeospatial::Ellipsoid::WGS84));
}
