// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "GeoTransformsNew.h"

#include "CesiumGeospatial/GlobeTransforms.h"
#include "CesiumRuntime.h"
#include "CesiumTransforms.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/quaternion.hpp>

using namespace CesiumGeospatial;

namespace {

LocalHorizontalCoordinateSystem createCoordinateSystem(
    const Ellipsoid& ellipsoid,
    const glm::dvec3& center,
    double scale) {
  return LocalHorizontalCoordinateSystem(
      center,
      LocalDirection::East,
      LocalDirection::South,
      LocalDirection::Up,
      1.0 / (scale * 100.0),
      ellipsoid);
}

} // namespace

GeoTransformsNew::GeoTransformsNew()
    : _coordinateSystem(
          glm::dvec3(0.0),
          LocalDirection::East,
          LocalDirection::North,
          LocalDirection::Up,
          1.0),
      _ellipsoid(CesiumGeospatial::Ellipsoid::WGS84),
      _center(0.0),
      _scale(1.0) {
  this->updateTransforms();
}

GeoTransformsNew::GeoTransformsNew(
    const CesiumGeospatial::Ellipsoid& ellipsoid,
    const glm::dvec3& center,
    double scale)
    : _coordinateSystem(
          glm::dvec3(0.0),
          LocalDirection::East,
          LocalDirection::North,
          LocalDirection::Up,
          1.0),
      _ellipsoid(ellipsoid),
      _center(center),
      _scale(scale) {
  this->updateTransforms();
}

void GeoTransformsNew::setCenter(const glm::dvec3& center) noexcept {
  if (this->_center != center) {
    this->_center = center;
    updateTransforms();
  }
}
void GeoTransformsNew::setEllipsoid(
    const CesiumGeospatial::Ellipsoid& ellipsoid) noexcept {
  if (this->_ellipsoid.getRadii() != ellipsoid.getRadii()) {
    this->_ellipsoid = ellipsoid;
    updateTransforms();
  }
}

glm::dquat GeoTransformsNew::ComputeSurfaceNormalRotation(
    const glm::dvec3& oldPosition,
    const glm::dvec3& newPosition) const {
  const glm::dvec3 oldEllipsoidNormal =
      this->ComputeGeodeticSurfaceNormal(oldPosition);
  const glm::dvec3 newEllipsoidNormal =
      this->ComputeGeodeticSurfaceNormal(newPosition);
  return glm::rotation(oldEllipsoidNormal, newEllipsoidNormal);
}

glm::dquat GeoTransformsNew::ComputeSurfaceNormalRotationUnreal(
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

void GeoTransformsNew::updateTransforms() noexcept {
  this->_coordinateSystem =
      createCoordinateSystem(this->_ellipsoid, this->_center, this->_scale);

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

glm::dvec3 GeoTransformsNew::TransformLongitudeLatitudeHeightToEcef(
    const glm::dvec3& longitudeLatitudeHeight) const noexcept {
  return _ellipsoid.cartographicToCartesian(
      CesiumGeospatial::Cartographic::fromDegrees(
          longitudeLatitudeHeight.x,
          longitudeLatitudeHeight.y,
          longitudeLatitudeHeight.z));
}

glm::dvec3 GeoTransformsNew::TransformEcefToLongitudeLatitudeHeight(
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

glm::dvec3 GeoTransformsNew::TransformLongitudeLatitudeHeightToUnreal(
    const glm::dvec3& origin,
    const glm::dvec3& longitudeLatitudeHeight) const noexcept {
  glm::dvec3 ecef =
      this->TransformLongitudeLatitudeHeightToEcef(longitudeLatitudeHeight);
  return this->TransformEcefToUnreal(origin, ecef);
}

glm::dvec3 GeoTransformsNew::TransformUnrealToLongitudeLatitudeHeight(
    const glm::dvec3& origin,
    const glm::dvec3& ue) const noexcept {
  glm::dvec3 ecef = this->TransformUnrealToEcef(origin, ue);
  return this->TransformEcefToLongitudeLatitudeHeight(ecef);
}

glm::dvec3 GeoTransformsNew::TransformEcefToUnreal(
    const glm::dvec3& origin,
    const glm::dvec3& ecef) const noexcept {
  return this->_coordinateSystem.ecefPositionToLocal(ecef) - origin;
}

glm::dvec3 GeoTransformsNew::TransformUnrealToEcef(
    const glm::dvec3& origin,
    const glm::dvec3& ue) const noexcept {
  return this->_coordinateSystem.localPositionToEcef(ue + origin);
}

glm::dquat GeoTransformsNew::TransformRotatorUnrealToEastSouthUp(
    const glm::dvec3& origin,
    const glm::dquat& UERotator,
    const glm::dvec3& ueLocation) const noexcept {
  glm::dmat3 esuToUe = this->ComputeEastSouthUpToUnreal(origin, ueLocation);
  glm::dmat3 ueToEsu = glm::affineInverse(esuToUe);
  glm::dquat ueToEsuQuat = glm::quat_cast(ueToEsu);
  return ueToEsuQuat * UERotator;
}

glm::dquat GeoTransformsNew::TransformRotatorEastSouthUpToUnreal(
    const glm::dvec3& origin,
    const glm::dquat& ESURotator,
    const glm::dvec3& ueLocation) const noexcept {

  glm::dmat3 esuToUe = this->ComputeEastSouthUpToUnreal(origin, ueLocation);
  glm::dquat esuToUeQuat = glm::quat_cast(esuToUe);
  return esuToUeQuat * ESURotator;
}

glm::dmat3 GeoTransformsNew::ComputeEastSouthUpToUnreal(
    const glm::dvec3& origin,
    const glm::dvec3& ue) const noexcept {
  glm::dvec3 ecef = this->TransformUnrealToEcef(origin, ue);
  LocalHorizontalCoordinateSystem newLocal =
      createCoordinateSystem(this->_ellipsoid, ecef, this->_scale);
  return glm::dmat3(
      newLocal.computeTransformationToAnotherLocal(this->_coordinateSystem));
}

glm::dmat3 GeoTransformsNew::ComputeEastNorthUpToEcef(
    const glm::dvec3& ecef) const noexcept {
  return glm::dmat3(CesiumGeospatial::GlobeTransforms::eastNorthUpToFixedFrame(
      ecef,
      _ellipsoid));
}
