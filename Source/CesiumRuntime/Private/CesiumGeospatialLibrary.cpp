// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumGeospatialLibrary.h"

#include "CesiumTransforms.h"
#include "Engine/World.h"
#include <CesiumGeospatial/Transforms.h>
#include <glm/trigonometric.hpp>

glm::dvec3 UCesiumGeospatialLibrary::TransformLongLatHeightToUnreal(
    const glm::dvec3& LongLatHeight,
    const glm::dmat4& EcefToUeAbsoluteWorld,
    const glm::dvec3& UeOriginLocation) {

  glm::dvec3 ecef = TransformLongLatHeightToEcef(LongLatHeight);

  return TransformEcefToUnreal(ecef, EcefToUeAbsoluteWorld, UeOriginLocation);
}

glm::dvec3 UCesiumGeospatialLibrary::TransformUnrealToLongLatHeight(
    const glm::dvec3& UeLocation,
    const glm::dmat4& UeAbsoluteWorldToEcef,
    const glm::dvec3& UeOriginLocation) {
  glm::dvec3 ecef = TransformUnrealToEcef(
      UeLocation,
      UeAbsoluteWorldToEcef,
      UeOriginLocation);
  return TransformEcefToLongLatHeight(ecef);
}

glm::dvec3 UCesiumGeospatialLibrary::TransformLongLatHeightToEcef(
    const glm::dvec3& LongLatHeight) {
  return CesiumGeospatial::Ellipsoid::WGS84.cartographicToCartesian(
      CesiumGeospatial::Cartographic::fromDegrees(
          LongLatHeight.x,
          LongLatHeight.y,
          LongLatHeight.z));
}

glm::dvec3
UCesiumGeospatialLibrary::TransformEcefToLongLatHeight(const glm::dvec3& Ecef) {
  std::optional<CesiumGeospatial::Cartographic> llh =
      CesiumGeospatial::Ellipsoid::WGS84.cartesianToCartographic(Ecef);
  if (!llh) {
    // TODO: since degenerate cases only happen close to Earth's center
    // would it make more sense to assign an arbitrary but correct LLH
    // coordinate to this case such as (0.0, 0.0, -_EARTH_RADIUS_)?
    return glm::dvec3(0.0, 0.0, 0.0);
  }
  return glm::dvec3(
      glm::degrees(llh->longitude),
      glm::degrees(llh->latitude),
      llh->height);
}

glm::dmat3 UCesiumGeospatialLibrary::TransformRotatorEastNorthUpToUnreal(
    const glm::dmat3& EnuRotation,
    const glm::dvec3& UeLocation,
    const glm::dmat4& UeAbsoluteWorldToEcef,
    const glm::dvec3& UeOriginLocation,
    const glm::dmat3& EcefToGeoreferenced) {

  glm::dmat3 enuToFixedUE = ComputeEastNorthUpToUnreal(
      UeLocation,
      UeAbsoluteWorldToEcef,
      UeOriginLocation,
      EcefToGeoreferenced);

  // Inverse to get East-North-Up To Unreal
  return glm::inverse(enuToFixedUE) * EnuRotation;
}

glm::dmat3 UCesiumGeospatialLibrary::TransformRotatorUnrealToEastNorthUp(
    const glm::dmat3& UeRotation,
    const glm::dvec3& UeLocation,
    const glm::dmat4& UeAbsoluteWorldToEcef,
    const glm::dvec3& UeOriginLocation,
    const glm::dmat3& EcefToGeoreferenced) {

  glm::dmat3 enuToFixedUE = ComputeEastNorthUpToUnreal(
      UeLocation,
      UeAbsoluteWorldToEcef,
      UeOriginLocation,
      EcefToGeoreferenced);

  return enuToFixedUE * UeRotation;
}

glm::dmat3 UCesiumGeospatialLibrary::ComputeEastNorthUpToUnreal(
    const glm::dvec3& UeLocation,
    const glm::dmat4& UeAbsoluteWorldToEcef,
    const glm::dvec3& UeOriginLocation,
    const glm::dmat3& EcefToGeoreferenced) {
  glm::dvec3 ecef = TransformUnrealToEcef(
      UeLocation,
      UeAbsoluteWorldToEcef,
      UeOriginLocation);
  glm::dmat3 enuToEcef = ComputeEastNorthUpToEcef(ecef);

  // Camera Axes = ENU
  // Unreal Axes = controlled by Georeference
  glm::dmat3 rotationCesium = glm::dmat3(EcefToGeoreferenced * enuToEcef);

  return glm::dmat3(CesiumTransforms::unrealToOrFromCesium) * rotationCesium *
         glm::dmat3(CesiumTransforms::unrealToOrFromCesium);
}

glm::dmat3
UCesiumGeospatialLibrary::ComputeEastNorthUpToEcef(const glm::dvec3& Ecef) {
  return glm::dmat3(
      CesiumGeospatial::Transforms::eastNorthUpToFixedFrame(Ecef));
}

glm::dvec3 UCesiumGeospatialLibrary::TransformEcefToUnreal(
    const glm::dvec3& EcefLocation,
    const glm::dmat4& EcefToUeAbsoluteWorld,
    const glm::dvec3& UeOriginLocation) {
  glm::dvec3 ueAbs = EcefToUeAbsoluteWorld * glm::dvec4(EcefLocation, 1.0);
  return ueAbs - UeOriginLocation;
}

glm::dvec3 UCesiumGeospatialLibrary::TransformUnrealToEcef(
    const glm::dvec3& UeLocation,
    const glm::dmat4& UeAbsoluteWorldToEcef,
    const glm::dvec3& UeOriginLocation) {
  glm::dvec4 ueAbs(UeLocation + UeOriginLocation, 1.0);
  return UeAbsoluteWorldToEcef * ueAbs;
}
