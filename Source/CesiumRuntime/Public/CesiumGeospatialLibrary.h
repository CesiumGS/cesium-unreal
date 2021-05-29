// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include <glm/mat3x3.hpp>

#include "CesiumGeospatialLibrary.generated.h"

/**
 *
 */
UCLASS()
class CESIUMRUNTIME_API UCesiumGeospatialLibrary : public UObject {
  GENERATED_BODY()

public:
  /**
   * Transforms the given WGS84 longitude in degrees (x), latitude in
   * degrees (y), and height in meters (z) into Unreal world coordinates
   * (relative to the floating origin).
   *
   * @param LongLatHeight The location longitude, latitude, and height.
   * @param EcefToUeAbsoluteWorld The transformation from ECEF to the Unreal
   * _absolute_ world origin.
   * @param UeOriginLocation The location of the Unreal _floating_ origin
   * relative to the _absolute_ world origin.
   * @return The converted WGS84 coordinates to Unreal coordinates, in relative
   * world space.
   */
  static glm::dvec3 TransformLongLatHeightToUnreal(
      const glm::dvec3& LongLatHeight,
      const glm::dmat4& EcefToUeAbsoluteWorld,
      const glm::dvec3& UeOriginLocation);

  /**
   * Transforms Unreal world coordinates (relative to the floating origin) into
   * WGS84 longitude in degrees (x), latitude in degrees (y), and height in
   * meters (z).
   *
   * @param UeLocation The Unreal coordinates, in relative world space.
   * @param UeAbsoluteWorldToEcef The transformation from the Unreal _absolute_
   * world origin to ECEF.
   * @param UeOriginLocation The location of the Unreal _floating_ point origin
   * relative to the _absolute_ world origin.
   * @return The WGS84 longitude, latitude, and height coordinates.
   */
  static glm::dvec3 TransformUnrealToLongLatHeight(
      const glm::dvec3& UeLocation,
      const glm::dmat4& UeAbsoluteWorldToEcef,
      const glm::dvec3& UeOriginLocation);

  /**
   * Transforms the given WGS84 longitude in degrees (x), latitude in
   * degrees (y), and height in meters (z) into Earth-Centered, Earth-Fixed
   * (ECEF) coordinates.
   *
   * @param LongLatHeight The location longitude, latitude, and height.
   * @return The Earth-Centered, Earth-Fixed (ECEF) coordinates.
   */
  static glm::dvec3
  TransformLongLatHeightToEcef(const glm::dvec3& LongLatHeight);

  /**
   * Transforms the given Earth-Centered, Earth-Fixed (ECEF) coordinates into
   * WGS84 longitude in degrees (x), latitude in degrees (y), and height in
   * meters (z).
   *
   * @param Ecef The specified location in Ecef coordinates.
   * @return The WGS longitude, latitude, and height coordinates.
   */
  static glm::dvec3 TransformEcefToLongLatHeight(const glm::dvec3& Ecef);

  /**
   * Transforms a rotation matrix from East-North-Up to Unreal world at the
   * given Unreal relative world location (relative to the floating origin).
   *
   * @param EnuRotation The rotation matrix in East-North-Up coordinates.
   * @param UeLocation The Unreal coordinates, in relative world space, at which
   * to perform the transformation.
   * @param UeAbsoluteWorldToEcef The transformation from the Unreal _absolute_
   * world origin to ECEF.
   * @param UeOriginLocation The location of the Unreal _floating_ origin
   * relative to the _absolute_ world origin.
   * @param EcefToGeoreferenced The transformation from ECEF to Georeferenced
   * reference frames. See {@link reference-frames.md}.
   * @return The rotation matrix in the Unreal reference frame.
   */
  static glm::dmat3 TransformRotatorEastNorthUpToUnreal(
      const glm::dmat3& EnuRotation,
      const glm::dvec3& UeLocation,
      const glm::dmat4& UeAbsoluteWorldToEcef,
      const glm::dvec3& UeOriginLocation,
      const glm::dmat3& EcefToGeoreferenced);

  /**
   * Transforms a rotation matrix from Unreal world to East-North-Up at the
   * given Unreal relative world location (relative to the floating origin).
   * @param UeRotation The rotation matrix in the Unreal reference frame.
   * @param UeLocation The Unreal coordinates, in relative world space, at which
   * to perform the transformation.
   * @param UeAbsoluteWorldToEcef The transformation from the Unreal _absolute_
   * world origin to ECEF.
   * @param UeOriginLocation The location of the Unreal _floating_ origin
   * relative to the _absolute_ world origin.
   * @param EcefToGeoreferenced The transformation from ECEF to Georeferenced
   * reference frames. See {@link reference-frames.md}
   * @return The rotation matrix in the East-North-Up reference frame.
   */
  static glm::dmat3 TransformRotatorUnrealToEastNorthUp(
      const glm::dmat3& UeRotation,
      const glm::dvec3& UeLocation,
      const glm::dmat4& UeAbsoluteWorldToEcef,
      const glm::dvec3& UeOriginLocation,
      const glm::dmat3& EcefToGeoreferenced);

  /**
   * Computes the rotation matrix from the local East-North-Up to Unreal at the
   * specified Unreal relative world location (relative to the floating
   * origin). The returned transformation works in Unreal's left-handed
   * coordinate system.
   *
   * @param UeLocation The Unreal coordinates, in relative world space, at which
   * to perform the transformation.
   * @param UeAbsoluteWorldToEcef The transformation from the Unreal _absolute_
   * world origin to ECEF.
   * @param UeOriginLocation The location of the Unreal _floating_ origin
   * relative to the _absolute_ world origin.
   * @param EcefToGeoreferenced The transformation from ECEF to Georeferenced
   * reference frames. See {@link reference-frames.md}
   * @return The rotation transformation from East-North-Up to Unreal.
   */
  static glm::dmat3 ComputeEastNorthUpToUnreal(
      const glm::dvec3& UeLocation,
      const glm::dmat4& UeAbsoluteWorldToEcef,
      const glm::dvec3& UeOriginLocation,
      const glm::dmat3& EcefToGeoreferenced);

  /**
   * Computes the rotation matrix from the local East-North-Up to
   * Earth-Centered, Earth-Fixed (ECEF) at the specified ECEF location.
   * @param Ecef The specified location in ECEF coordinates.
   * @return The rotation transformation from East-North-Up to ECEF.
   */
  static glm::dmat3 ComputeEastNorthUpToEcef(const glm::dvec3& Ecef);

  /**
   * Transforms the given point from Earth-Centered, Earth-Fixed (ECEF) into
   * Unreal relative world (relative to the floating origin).
   * @param EcefLocation The ECEF coordinate to transform
   * @param EcefToUeAbsoluteWorld The transform from ECEF to Unreal Absolute
   * World for a given georeference.
   * @param UeOriginLocation The location of the Unreal relative frame's
   * floating origin.
   * @return The location in Unreal coordinates in relative world space.
   */
  static glm::dvec3 TransformEcefToUnreal(
      const glm::dvec3& EcefLocation,
      const glm::dmat4& EcefToUeAbsoluteWorld,
      const glm::dvec3& UeOriginLocation);

  /**
   * Transforms the given point from Unreal relative world (relative to the
   * floating origin) to Earth-Centered, Earth-Fixed (ECEF).
   * @param UeLocation The Unreal coordinate in relative world space.
   * @param UeAbsoluteWorldToEcef The transform from Unreal's absolute world
   * origin to ECEF.
   * @param UeOriginLocation The location of the Unreal relative frame's
   * floating origin.
   * @return The location in ECEF coordinates.
   */
  static glm::dvec3 TransformUnrealToEcef(
      const glm::dvec3& UeLocation,
      const glm::dmat4& UeAbsoluteWorldToEcef,
      const glm::dvec3& UeOriginLocation);

private:
  static CesiumGeospatial::Ellipsoid _ellipsoid;
};
