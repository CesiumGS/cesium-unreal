// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CesiumGeospatial/Ellipsoid.h"

#include <glm/glm.hpp>

/**
 * @brief A lightweight structure to encapsulate coordinate transforms.
 *
 * It encapsulates the conversions between...
 * - Earth-Centered, Earth-Fixed (ECEF) coordinates
 * - Georeferenced coordinates (Latitude/Longitude/Height)
 * - Unreal coordinates (relative to the unreal world origin)
 *
 */
class CESIUMRUNTIME_API GeoTransforms {

public:
  /**
   * @brief Creates a new instance
   */
  GeoTransforms()
      : _ellipsoid(CesiumGeospatial::Ellipsoid::WGS84),
        _center(glm::dvec3(0.0)),
        _georeferencedToEcef(1.0),
        _ecefToGeoreferenced(1.0),
        _ueAbsToEcef(1.0),
        _ecefToUeAbs(1.0) {
    updateTransforms();
  }

  /**
   * @brief Creates a new instance.
   *
   * The center position is the position of the origin of the
   * local coordinate system that is established by this instance.
   *
   * @param ellipsoid The ellipsoid to use for the georeferenced coordinates
   * @param center The center position.
   */
  GeoTransforms(
      const CesiumGeospatial::Ellipsoid& ellipsoid,
      const glm::dvec3& center)
      : _ellipsoid(ellipsoid),
        _center(center),
        _georeferencedToEcef(1.0),
        _ecefToGeoreferenced(1.0),
        _ueAbsToEcef(1.0),
        _ecefToUeAbs(1.0) {
    updateTransforms();
  }

  /**
   * @brief Set the center position of this instance
   *
   * The center position is the position of the origin of the
   * local coordinate system that is established by this instance.
   *
   * @param center The center position.
   */
  void setCenter(const glm::dvec3& center) noexcept;

  /**
   * @brief Set the ellipsoid of this instance
   *
   * @param ellipsoid The ellipsoid
   */
  void setEllipsoid(const CesiumGeospatial::Ellipsoid& ellipsoid) noexcept;

  /**
   * Transforms the given longitude in degrees (x), latitude in
   * degrees (y), and height in meters (z) into Earth-Centered, Earth-Fixed
   * (ECEF) coordinates.
   */
  glm::dvec3 TransformLongitudeLatitudeHeightToEcef(
      const glm::dvec3& LongitudeLatitudeHeight) const noexcept;

  /**
   * Transforms the given Earth-Centered, Earth-Fixed (ECEF) coordinates into
   * longitude in degrees (x), latitude in degrees (y), and height in
   * meters (z).
   */
  glm::dvec3
  TransformEcefToLongitudeLatitudeHeight(const glm::dvec3& Ecef) const noexcept;

  /**
   * Transforms the given longitude in degrees (x), latitude in
   * degrees (y), and height in meters (z) into Unreal world coordinates
   * (relative to the floating origin).
   */
  glm::dvec3 TransformLongitudeLatitudeHeightToUnreal(
      const glm::dvec3& origin,
      const glm::dvec3& LongitudeLatitudeHeight) const noexcept;

  /**
   * Transforms Unreal world coordinates (relative to the floating origin) into
   * longitude in degrees (x), latitude in degrees (y), and height in
   * meters (z).
   */
  glm::dvec3 TransformUnrealToLongitudeLatitudeHeight(
      const glm::dvec3& origin,
      const glm::dvec3& Ue) const noexcept;

  /**
   * Transforms the given point from Earth-Centered, Earth-Fixed (ECEF) into
   * Unreal world coordinates (relative to the floating origin).
   */
  glm::dvec3 TransformEcefToUnreal(
      const glm::dvec3& origin,
      const glm::dvec3& Ecef) const noexcept;

  /**
   * Transforms the given point from Unreal world coordinates (relative to the
   * floating origin) to Earth-Centered, Earth-Fixed (ECEF).
   */
  glm::dvec3 TransformUnrealToEcef(
      const glm::dvec3& origin,
      const glm::dvec3& Ue) const noexcept;

  /**
   * Transforms a rotator from Unreal world to East-North-Up at the given
   * Unreal relative world location (relative to the floating origin).
   */
  glm::dquat TransformRotatorUnrealToEastNorthUp(
      const glm::dvec3& origin,
      const glm::dquat& UeRotator,
      const glm::dvec3& UeLocation) const noexcept;

  /**
   * Transforms a rotator from East-North-Up to Unreal world at the given
   * Unreal world location (relative to the floating origin).
   */
  glm::dquat TransformRotatorEastNorthUpToUnreal(
      const glm::dvec3& origin,
      const glm::dquat& EnuRotator,
      const glm::dvec3& UeLocation) const noexcept;

  /**
   * Computes the rotation matrix from the local East-North-Up to Unreal at the
   * specified Unreal world location (relative to the floating
   * origin). The returned transformation works in Unreal's left-handed
   * coordinate system.
   */
  glm::dmat3 ComputeEastNorthUpToUnreal(
      const glm::dvec3& origin,
      const glm::dvec3& Ue) const noexcept;

  /**
   * Computes the rotation matrix from the local East-North-Up to
   * Earth-Centered, Earth-Fixed (ECEF) at the specified ECEF location.
   */
  glm::dmat3 ComputeEastNorthUpToEcef(const glm::dvec3& Ecef) const noexcept;

  /*
   * GEOREFERENCE TRANSFORMS
   */

  /**
   * @brief Gets the transformation from the "Georeferenced" reference frame
   * defined by this instance to the "Ellipsoid-centered" reference frame (i.e.
   * ECEF).
   *
   * Gets a matrix that transforms coordinates from the "Georeference" reference
   * frame defined by this instance to the "Ellipsoid-centered" reference frame,
   * which is usually Earth-centered, Earth-fixed. See {@link
   * reference-frames.md}.
   */
  const glm::dmat4&
  GetGeoreferencedToEllipsoidCenteredTransform() const noexcept {
    return this->_georeferencedToEcef;
  }

  /**
   * @brief Gets the transformation from the "Ellipsoid-centered" reference
   * frame (i.e. ECEF) to the georeferenced reference frame defined by this
   * instance.
   *
   * Gets a matrix that transforms coordinates from the "Ellipsoid-centered"
   * reference frame (which is usually Earth-centered, Earth-fixed) to the
   * "Georeferenced" reference frame defined by this instance. See {@link
   * reference-frames.md}.
   */
  const glm::dmat4&
  GetEllipsoidCenteredToGeoreferencedTransform() const noexcept {
    return this->_ecefToGeoreferenced;
  }

  /**
   * @brief Gets the transformation from the _absolute_ "Unreal World" reference
   * frame to the "Ellipsoid-centered" reference frame (i.e. ECEF).
   *
   * Gets a matrix that transforms coordinates from the absolute "Unreal World"
   * reference frame (with respect to the absolute world origin, not the
   * floating origin) to the "Ellipsoid-centered" reference frame (which is
   * usually Earth-centered, Earth-fixed). See {@link reference-frames.md}.
   */
  const glm::dmat4&
  GetAbsoluteUnrealWorldToEllipsoidCenteredTransform() const noexcept {
    return this->_ueAbsToEcef;
  }

  /**
   * @brief Gets the transformation from the "Ellipsoid-centered" reference
   * frame (i.e. ECEF) to the absolute "Unreal World" reference frame.
   *
   * Gets a matrix that transforms coordinates from the "Ellipsoid-centered"
   * reference frame (which is usually Earth-centered, Earth-fixed) to the
   * absolute "Unreal world" reference frame (with respect to the absolute world
   * origin, not the floating origin). See {@link reference-frames.md}.
   */
  const glm::dmat4&
  GetEllipsoidCenteredToAbsoluteUnrealWorldTransform() const noexcept {
    return this->_ecefToUeAbs;
  }

  /**
   * @brief Computes the normal of the plane tangent to the surface of the
   * ellipsoid that is used by this instance, at the provided position.
   *
   * @param position The cartesian position for which to to determine the
   * surface normal.
   * @return The normal.
   */
  glm::dvec3 ComputeGeodeticSurfaceNormal(const glm::dvec3& position) const {
    return _ellipsoid.geodeticSurfaceNormal(position);
  }

private:
  /**
   * Update the derived state (i.e. the matrices) when either
   * the center or the ellipsoid has changed.
   */
  void updateTransforms() noexcept;

  // Modifiable state
  CesiumGeospatial::Ellipsoid _ellipsoid;
  glm::dvec3 _center;

  // Derived state
  glm::dmat4 _georeferencedToEcef;
  glm::dmat4 _ecefToGeoreferenced;
  glm::dmat4 _ueAbsToEcef;
  glm::dmat4 _ecefToUeAbs;
};
