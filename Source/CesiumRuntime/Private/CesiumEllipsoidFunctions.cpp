// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumEllipsoidFunctions.h"
#include "VecMath.h"
#include <CesiumGeospatial/Ellipsoid.h>
#include <CesiumGeospatial/GlobeTransforms.h>
#include <CesiumUtility/Math.h>

using namespace CesiumGeospatial;
using namespace CesiumUtility;

FVector CesiumEllipsoidFunctions::ScaleToGeodeticSurface(
    const CesiumGeospatial::Ellipsoid& Ellipsoid,
    const FVector& EllipsoidCenteredEllipsoidFixedPosition) {
  std::optional<glm::dvec3> result = Ellipsoid.scaleToGeodeticSurface(
      VecMath::createVector3D(EllipsoidCenteredEllipsoidFixedPosition));
  if (result) {
    return VecMath::createVector(*result);
  } else {
    return FVector(0.0, 0.0, 0.0);
  }
}

FVector CesiumEllipsoidFunctions::GeodeticSurfaceNormal(
    const CesiumGeospatial::Ellipsoid& Ellipsoid,
    const FVector& EllipsoidCenteredEllipsoidFixedPosition) {
  return VecMath::createVector(Ellipsoid.geodeticSurfaceNormal(
      VecMath::createVector3D(EllipsoidCenteredEllipsoidFixedPosition)));
}

FVector CesiumEllipsoidFunctions::
    LongitudeLatitudeHeightToEllipsoidCenteredEllipsoidFixed(
        const CesiumGeospatial::Ellipsoid& Ellipsoid,
        const FVector& LongitudeLatitudeHeight) {
  glm::dvec3 cartesian =
      Ellipsoid.cartographicToCartesian(Cartographic::fromDegrees(
          LongitudeLatitudeHeight.X,
          LongitudeLatitudeHeight.Y,
          LongitudeLatitudeHeight.Z));
  return VecMath::createVector(cartesian);
}

FVector CesiumEllipsoidFunctions::
    EllipsoidCenteredEllipsoidFixedToLongitudeLatitudeHeight(
        const CesiumGeospatial::Ellipsoid& Ellipsoid,
        const FVector& EllipsoidCenteredEllipsoidFixedPosition) {
  std::optional<Cartographic> result = Ellipsoid.cartesianToCartographic(
      VecMath::createVector3D(EllipsoidCenteredEllipsoidFixedPosition));
  if (result) {
    return FVector(
        Math::radiansToDegrees(result->longitude),
        Math::radiansToDegrees(result->latitude),
        result->height);
  } else {
    return FVector(0.0, 0.0, 0.0);
  }
}

FMatrix CesiumEllipsoidFunctions::EastNorthUpToEllipsoidCenteredEllipsoidFixed(
    const CesiumGeospatial::Ellipsoid& Ellipsoid,
    const FVector& EllipsoidCenteredEllipsoidFixedPosition) {
  return VecMath::createMatrix(
      CesiumGeospatial::GlobeTransforms::eastNorthUpToFixedFrame(
          VecMath::createVector3D(EllipsoidCenteredEllipsoidFixedPosition),
          Ellipsoid));
}
