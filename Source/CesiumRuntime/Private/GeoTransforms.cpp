// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "GeoTransforms.h"

#include "CesiumGeospatial/Transforms.h"
#include "CesiumTransforms.h"
#include "VecMath.h"

// ONLY used for logging!
#include "CesiumRuntime.h"

#include "GeoTransforms.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/quaternion.hpp>

void GeoTransforms::setCenter(const glm::dvec3& center) noexcept {
  if (this->_center != center) {
    this->_center = center;
    updateTransforms();
  }
}
void GeoTransforms::setEllipsoid(
    const CesiumGeospatial::Ellipsoid& ellipsoid) noexcept {
  if (this->_ellipsoid.getRadii() != ellipsoid.getRadii()) {
    this->_ellipsoid = ellipsoid;
    updateTransforms();
  }
}

glm::dquat GeoTransforms::ComputeSurfaceNormalRotation(
    const glm::dvec3& oldPosition,
    const glm::dvec3& newPosition) const {
  const glm::dvec3 oldEllipsoidNormal =
      this->ComputeGeodeticSurfaceNormal(oldPosition);
  const glm::dvec3 newEllipsoidNormal =
      this->ComputeGeodeticSurfaceNormal(newPosition);
  return glm::rotation(oldEllipsoidNormal, newEllipsoidNormal);
}

glm::dquat GeoTransforms::ComputeSurfaceNormalRotationUnreal(
    const glm::dvec3& oldPosition,
    const glm::dvec3& newPosition) const {
  const glm::dmat3 ecefToUnreal =
      glm::dmat3(this->GetEllipsoidCenteredToAbsoluteUnrealWorldTransform());
  const glm::dvec3 oldEllipsoidNormalUnreal = glm::normalize(
      ecefToUnreal * this->ComputeGeodeticSurfaceNormal(oldPosition));
  const glm::dvec3 newEllipsoidNormalUnreal = glm::normalize(
      ecefToUnreal * this->ComputeGeodeticSurfaceNormal(newPosition));
  return glm::rotation(oldEllipsoidNormalUnreal, newEllipsoidNormalUnreal);
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

  UE_LOG(
      LogCesium,
      Verbose,
      TEXT(
          "GeoTransforms::updateTransforms with center %f %f %f and ellipsoid radii %f %f %f"),
      _center.x,
      _center.y,
      _center.z,
      _ellipsoid.getRadii().x,
      _ellipsoid.getRadii().y,
      _ellipsoid.getRadii().z);
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
  glm::dvec3 ueAbs = glm::dvec3(this->_ecefToUeAbs * glm::dvec4(ecef, 1.0));
  return ueAbs - origin;
}

glm::dvec3 GeoTransforms::TransformUnrealToEcef(
    const glm::dvec3& origin,
    const glm::dvec3& ue) const noexcept {

  glm::dvec3 ueAbs = ue + origin;
  return glm::dvec3(this->_ueAbsToEcef * glm::dvec4(ueAbs, 1.0));
}

glm::dquat GeoTransforms::TransformRotatorUnrealToEastSouthUp(
    const glm::dvec3& origin,
    const glm::dquat& UERotator,
    const glm::dvec3& ueLocation) const noexcept {
  glm::dmat3 esuToUe = this->ComputeEastSouthUpToUnreal(origin, ueLocation);
  glm::dmat3 ueToEsu = glm::affineInverse(esuToUe);
  glm::dquat ueToEsuQuat = glm::quat_cast(ueToEsu);
  return ueToEsuQuat * UERotator;
}

glm::dquat GeoTransforms::TransformRotatorEastSouthUpToUnreal(
    const glm::dvec3& origin,
    const glm::dquat& ESURotator,
    const glm::dvec3& ueLocation) const noexcept {

  glm::dmat3 esuToUe = this->ComputeEastSouthUpToUnreal(origin, ueLocation);
  glm::dquat esuToUeQuat = glm::quat_cast(esuToUe);
  return esuToUeQuat * ESURotator;
}

glm::dmat3 GeoTransforms::ComputeEastSouthUpToUnreal(
    const glm::dvec3& origin,
    const glm::dvec3& ue) const noexcept {
  glm::dvec3 ecef = this->TransformUnrealToEcef(origin, ue);
  glm::dmat3 enuToEcef = this->ComputeEastNorthUpToEcef(ecef);

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
