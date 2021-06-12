// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "GeoTransforms.h"

#include "CesiumGeospatial/Transforms.h"
#include "CesiumTransforms.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/quaternion.hpp>

void GeoTransforms::setCenter(const glm::dvec3& center) noexcept {
  this->_center = center;
  updateTransforms();
}
void GeoTransforms::setEllipsoid(const CesiumGeospatial::Ellipsoid& ellipsoid)  noexcept {
  this->_ellipsoid = ellipsoid;
  updateTransforms();
}

void GeoTransforms::updateTransforms() noexcept {
  this->_georeferencedToEcef =
      CesiumGeospatial::Transforms::eastNorthUpToFixedFrame(
          _center,
          _ellipsoid);
  this->_ecefToGeoreferenced = glm::affineInverse(this->_georeferencedToEcef);
  this->_ueAbsToEcef = this->_georeferencedToEcef *
                       CesiumTransforms::scaleToCesium *
                       CesiumTransforms::unrealToOrFromCesium;
  this->_ecefToUeAbs = CesiumTransforms::unrealToOrFromCesium *
                       CesiumTransforms::scaleToUnrealWorld *
                       this->_ecefToGeoreferenced;
}

glm::dvec3 GeoTransforms::TransformLongitudeLatitudeHeightToEcef(
    const glm::dvec3& longitudeLatitudeHeight) const noexcept {
  return _ellipsoid.cartographicToCartesian(
      CesiumGeospatial::Cartographic::fromDegrees(
          longitudeLatitudeHeight.x,
          longitudeLatitudeHeight.y,
          longitudeLatitudeHeight.z));
}

glm::dvec3 GeoTransforms::TransformEcefToLongitudeLatitudeHeight(
    const glm::dvec3& ecef) const noexcept {
  std::optional<CesiumGeospatial::Cartographic> llh =
      _ellipsoid.cartesianToCartographic(ecef);
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

glm::dvec3 GeoTransforms::TransformLongitudeLatitudeHeightToUnreal(
    const glm::dvec3& origin,
    const glm::dvec3& longitudeLatitudeHeight) const noexcept {
  glm::dvec3 ecef =
      this->TransformLongitudeLatitudeHeightToEcef(longitudeLatitudeHeight);
  return this->TransformEcefToUnreal(origin, ecef);
}

glm::dvec3 GeoTransforms::TransformUnrealToLongitudeLatitudeHeight(
    const glm::dvec3& origin,
    const glm::dvec3& ue) const noexcept {
  glm::dvec3 ecef = this->TransformUnrealToEcef(origin, ue);
  return this->TransformEcefToLongitudeLatitudeHeight(ecef);
}

glm::dvec3 GeoTransforms::TransformEcefToUnreal(
    const glm::dvec3& origin,
    const glm::dvec3& ecef) const noexcept {
  glm::dvec3 ueAbs = this->_ecefToUeAbs * glm::dvec4(ecef, 1.0);
  return ueAbs - origin;
}

glm::dvec3 GeoTransforms::TransformUnrealToEcef(
    const glm::dvec3& origin,
    const glm::dvec3& ue) const noexcept {

  glm::dvec3 ueAbs = ue + origin;
  return this->_ueAbsToEcef * glm::dvec4(ueAbs, 1.0);
}

glm::dquat GeoTransforms::TransformRotatorUnrealToEastNorthUp(
    const glm::dquat& UERotator,
    const glm::dvec3& ueLocation) const noexcept {
  /* TODO NOT IMPLEMENTED YET
  glm::dmat3 enuToFixedUE = this->ComputeEastNorthUpToUnreal(ueLocation);
  FMatrix enuAdjustmentMatrix(
      FVector(enuToFixedUE[0].x, enuToFixedUE[0].y, enuToFixedUE[0].z),
      FVector(enuToFixedUE[1].x, enuToFixedUE[1].y, enuToFixedUE[1].z),
      FVector(enuToFixedUE[2].x, enuToFixedUE[2].y, enuToFixedUE[2].z),
      FVector::ZeroVector);
  return FRotator(enuAdjustmentMatrix.ToQuat() * UERotator.Quaternion());
  */
  return UERotator;
}

glm::dquat GeoTransforms::TransformRotatorEastNorthUpToUnreal(
    const glm::dquat& ENURotator,
    const glm::dvec3& ueLocation) const noexcept {
  /* TODO NOT IMPLEMENTED YET
  glm::dmat3 enuToFixedUE = this->ComputeEastNorthUpToUnreal(ueLocation);
  FMatrix enuAdjustmentMatrix(
      FVector(enuToFixedUE[0].x, enuToFixedUE[0].y, enuToFixedUE[0].z),
      FVector(enuToFixedUE[1].x, enuToFixedUE[1].y, enuToFixedUE[1].z),
      FVector(enuToFixedUE[2].x, enuToFixedUE[2].y, enuToFixedUE[2].z),
      FVector::ZeroVector);

  FMatrix inverse = enuAdjustmentMatrix.InverseFast();
  return FRotator(inverse.ToQuat() * ENURotator.Quaternion());
  */
  return ENURotator;
}

glm::dmat3 GeoTransforms::ComputeEastNorthUpToUnreal(
    const glm::dvec3& origin,
    const glm::dvec3& ue) const noexcept {
  glm::dvec3 ecef = this->TransformUnrealToEcef(origin, ue);
  glm::dmat3 enuToEcef = this->ComputeEastNorthUpToEcef(ecef);

  // Camera Axes = ENU
  // Unreal Axes = controlled by Georeference
  glm::dmat3 rotationCesium =
      glm::dmat3(this->_ecefToGeoreferenced) * enuToEcef;

  return glm::dmat3(CesiumTransforms::unrealToOrFromCesium) * rotationCesium *
         glm::dmat3(CesiumTransforms::unrealToOrFromCesium);
}

glm::dmat3
GeoTransforms::ComputeEastNorthUpToEcef(const glm::dvec3& ecef) const noexcept {
  return glm::dmat3(
      CesiumGeospatial::Transforms::eastNorthUpToFixedFrame(ecef, _ellipsoid));
}
