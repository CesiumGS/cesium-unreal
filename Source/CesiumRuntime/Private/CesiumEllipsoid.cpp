// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumEllipsoid.h"
#include "CesiumEllipsoidFunctions.h"
#include "VecMath.h"
#include <CesiumGeospatial/Ellipsoid.h>

using namespace CesiumGeospatial;

FVector UCesiumEllipsoid::GetRadii() { return this->Radii; }

double UCesiumEllipsoid::GetMaximumRadius() { return this->Radii.X; }

double UCesiumEllipsoid::GetMinimumRadius() { return this->Radii.Z; }

FVector UCesiumEllipsoid::ScaleToGeodeticSurface(
    const FVector& EllipsoidCenteredEllipsoidFixedPosition) {
  return CesiumEllipsoidFunctions::ScaleToGeodeticSurface(
      *this->GetNativeEllipsoid(),
      EllipsoidCenteredEllipsoidFixedPosition);
}

FVector UCesiumEllipsoid::GeodeticSurfaceNormal(
    const FVector& EllipsoidCenteredEllipsoidFixedPosition) {
  return CesiumEllipsoidFunctions::GeodeticSurfaceNormal(
      *this->GetNativeEllipsoid(),
      EllipsoidCenteredEllipsoidFixedPosition);
}

FVector
UCesiumEllipsoid::LongitudeLatitudeHeightToEllipsoidCenteredEllipsoidFixed(
    const FVector& LongitudeLatitudeHeight) {
  return CesiumEllipsoidFunctions::
      LongitudeLatitudeHeightToEllipsoidCenteredEllipsoidFixed(
          *this->GetNativeEllipsoid(),
          LongitudeLatitudeHeight);
}

FVector
UCesiumEllipsoid::EllipsoidCenteredEllipsoidFixedToLongitudeLatitudeHeight(
    const FVector& EllipsoidCenteredEllipsoidFixedPosition) {
  return CesiumEllipsoidFunctions::
      EllipsoidCenteredEllipsoidFixedToLongitudeLatitudeHeight(
          *this->GetNativeEllipsoid(),
          EllipsoidCenteredEllipsoidFixedPosition);
}

FMatrix UCesiumEllipsoid::EastNorthUpToEllipsoidCenteredEllipsoidFixed(
    const FVector& EllipsoidCenteredEllipsoidFixedPosition) {
  return CesiumEllipsoidFunctions::EastNorthUpToEllipsoidCenteredEllipsoidFixed(
      *this->GetNativeEllipsoid(),
      EllipsoidCenteredEllipsoidFixedPosition);
}

LocalHorizontalCoordinateSystem
UCesiumEllipsoid::CreateCoordinateSystem(const FVector& Center, double Scale) {
  {
    return LocalHorizontalCoordinateSystem(
        VecMath::createVector3D(Center),
        LocalDirection::East,
        LocalDirection::South,
        LocalDirection::Up,
        1.0 / Scale,
        *this->GetNativeEllipsoid());
  }
}

TSharedPtr<Ellipsoid> UCesiumEllipsoid::GetNativeEllipsoid() {
  if (!this->NativeEllipsoid.IsValid()) {
    /*this->NativeEllipsoid = MakeShared<CesiumGeospatial::Ellipsoid>(
        this->Radii.X,
        this->Radii.Y,
        this->Radii.Z);*/
    this->NativeEllipsoid = MakeShared<Ellipsoid>(
        Ellipsoid::WGS84.getRadii().x,
        Ellipsoid::WGS84.getRadii().y,
        Ellipsoid::WGS84.getRadii().z);
  }

  return this->NativeEllipsoid;
}
