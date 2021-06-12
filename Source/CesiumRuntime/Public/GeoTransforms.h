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
   * @brief Creates a new instance
   *
   * @param ellipsoid The ellipsoid to use for the georeferenced coordinates
   * @param center The center position (TODO Explain...)
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

  /** @brief Destructor */
  ~GeoTransforms() {}

  /** @brief Copy constructor */
  GeoTransforms(const GeoTransforms& other)
      : GeoTransforms(
            other._ellipsoid,
            other._center,
            other._georeferencedToEcef,
            other._ecefToGeoreferenced,
            other._ueAbsToEcef,
            other._ecefToUeAbs) {}

  /**
   * @brief Set the center position of this instance
   *
   * @param center The center position (TODO Explain...)
   */
  void setCenter(const glm::dvec3& center) noexcept;

  /**
   * @brief Returns the center of this instance
   * 
   * @return The center
   */
  /* // TODO Not required yet?
  const glm::dvec3& getCenter() const noexcept {
    return this->_center; 
  }
  */

  /**
   * @brief Set the ellipsoid of this instance
   *
   * @param ellipsoid The ellipsoid
   */
  void setEllipsoid(const CesiumGeospatial::Ellipsoid& ellipsoid) noexcept;

  /**
   * @brief Returns the ellipsoid of this instance
   * 
   * @return The ellipsoid
   */
  /* // TODO Not required yet?
  const CesiumGeospatial::Ellipsoid& getEllipsoid() const noexcept {
    return this->_ellipsoid;
  }
  */

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
  glm::dvec3 TransformLongitudeLatitudeHeightToUe(
      const glm::dvec3& origin,
      const glm::dvec3& LongitudeLatitudeHeight) const noexcept;

  /**
   * Transforms Unreal world coordinates (relative to the floating origin) into
   * longitude in degrees (x), latitude in degrees (y), and height in
   * meters (z).
   */
  glm::dvec3 TransformUeToLongitudeLatitudeHeight(
      const glm::dvec3& origin,
      const glm::dvec3& Ue) const noexcept;

  /**
   * Transforms the given point from Earth-Centered, Earth-Fixed (ECEF) into
   * Unreal relative world (relative to the floating origin).
   */
  glm::dvec3
  TransformEcefToUe(const glm::dvec3& origin, const glm::dvec3& Ecef) const noexcept;

  /**
   * Transforms the given point from Unreal relative world (relative to the
   * floating origin) to Earth-Centered, Earth-Fixed (ECEF).
   */
  glm::dvec3
  TransformUeToEcef(const glm::dvec3& origin, const glm::dvec3& Ue) const noexcept;

  /**
   * Transforms a rotator from Unreal world to East-North-Up at the given
   * Unreal relative world location (relative to the floating origin).
   */
  glm::dquat TransformRotatorUeToEnu(
      const glm::dquat& UeRotator,
      const glm::dvec3& UeLocation) const noexcept;

  /**
   * Transforms a rotator from East-North-Up to Unreal world at the given
   * Unreal relative world location (relative to the floating origin).
   */
  glm::dquat TransformRotatorEnuToUe(
      const glm::dquat& EnuRotator,
      const glm::dvec3& UeLocation) const noexcept;

  /**
   * Computes the rotation matrix from the local East-North-Up to Unreal at the
   * specified Unreal relative world location (relative to the floating
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
  const glm::dmat4& GetGeoreferencedToEllipsoidCenteredTransform() const noexcept {
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
  const glm::dmat4& GetEllipsoidCenteredToGeoreferencedTransform() const noexcept {
    return this->_ecefToGeoreferenced;
  }

  /**
   * @brief Gets the transformation from the "Unreal World" reference frame to
   * the "Ellipsoid-centered" reference frame (i.e. ECEF).
   *
   * Gets a matrix that transforms coordinates from the "Unreal World" reference
   * frame (with respect to the absolute world origin, not the floating origin)
   * to the "Ellipsoid-centered" reference frame (which is usually
   * Earth-centered, Earth-fixed). See {@link reference-frames.md}.
   */
  const glm::dmat4& GetUnrealWorldToEllipsoidCenteredTransform() const noexcept{
    return this->_ueAbsToEcef;
  }

  /**
   * @brief Gets the transformation from the "Ellipsoid-centered" reference
   * frame (i.e. ECEF) to the "Unreal World" reference frame.
   *
   * Gets a matrix that transforms coordinates from the "Ellipsoid-centered"
   * reference frame (which is usually Earth-centered, Earth-fixed) to the
   * "Unreal world" reference frame (with respect to the absolute world origin,
   * not the floating origin). See {@link reference-frames.md}.
   */
  const glm::dmat4& GetEllipsoidCenteredToUnrealWorldTransform() const noexcept {
    return this->_ecefToUeAbs;
  }

private:
  /**
   * @brief Creates a new instance (for copy constructor).
   * TODO The params are all fields. I think this is NOT auto-generated
   * by the compiler when there is another constructor...
   */
  GeoTransforms(
      const CesiumGeospatial::Ellipsoid& ellipsoid,
      const glm::dvec3& center,
      const glm::dmat4 georeferencedToEcef,
      const glm::dmat4 ecefToGeoreferenced,
      const glm::dmat4 ueAbsToEcef,
      const glm::dmat4 ecefToUeAbs)
      : _ellipsoid(CesiumGeospatial::Ellipsoid::WGS84),
        _center(center),
        _georeferencedToEcef(georeferencedToEcef),
        _ecefToGeoreferenced(ecefToGeoreferenced),
        _ueAbsToEcef(ueAbsToEcef),
        _ecefToUeAbs(ecefToUeAbs) {
    // No need to call updateTransforms here
  }

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
