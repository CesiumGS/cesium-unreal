// Fill out your copyright notice in the Description page of Project Settings.

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
   */
  static glm::dvec3 TransformLongLatHeightToUnreal(
      const glm::dvec3& LongLatHeight,
      const glm::dmat4& EcefToUeAbsoluteWorld,
      const glm::dvec3& UeOriginLocation);

  /**
   * Transforms Unreal world coordinates (relative to the floating origin) into
   * WGS84 longitude in degrees (x), latitude in degrees (y), and height in
   * meters (z).
   */
  static glm::dvec3 TransformUnrealToLongLatHeight(
      const glm::dvec3& Ue,
      const glm::dmat4& UeAbsoluteWorldToEcef,
      const glm::dvec3& UeOriginLocation);

  /**
   * Transforms the given WGS84 longitude in degrees (x), latitude in
   * degrees (y), and height in meters (z) into Earth-Centered, Earth-Fixed
   * (ECEF) coordinates.
   */
  static glm::dvec3
  TransformLongLatHeightToEcef(const glm::dvec3& LongLatHeight);

  /**
   * Transforms the given Earth-Centered, Earth-Fixed (ECEF) coordinates into
   * WGS84 longitude in degrees (x), latitude in degrees (y), and height in
   * meters (z).
   */
  static glm::dvec3 TransformEcefToLongLatHeight(const glm::dvec3& Ecef);

  /**
   * Transforms a rotator from East-North-Up to Unreal world at the given
   * Unreal relative world location (relative to the floating origin).
   */
  static glm::dmat3 TransformRotatorEastNorthUpToUnreal(
      const glm::dmat3& EnuRotator,
      const glm::dvec3& UeLocation,
      const glm::dmat4& UeAbsoluteWorldToEcef,
      const glm::dvec3& UeOriginLocation,
      const glm::dmat3& EcefToGeoreferenced);

  /**
   * Transforms a rotator from Unreal world to East-North-Up at the given
   * Unreal relative world location (relative to the floating origin).
   */
  static glm::dmat3 TransformRotatorUnrealToEastNorthUp(
      const glm::dmat3& UeRotator,
      const glm::dvec3& UeLocation,
      const glm::dmat4& UeAbsoluteWorldToEcef,
      const glm::dvec3& UeOriginLocation,
      const glm::dmat3& EcefToGeoreferenced);

  /**
   * Computes the rotation matrix from the local East-North-Up to Unreal at the
   * specified Unreal relative world location (relative to the floating
   * origin). The returned transformation works in Unreal's left-handed
   * coordinate system.
   */
  static glm::dmat3 ComputeEastNorthUpToUnreal(
      const glm::dvec3& Ue,
      const glm::dmat4& UeAbsoluteWorldToEcef,
      const glm::dvec3& UeOriginLocation,
      const glm::dmat3& EcefToGeoreferenced);

  /**
   * Computes the rotation matrix from the local East-North-Up to
   * Earth-Centered, Earth-Fixed (ECEF) at the specified ECEF location.
   */
  static glm::dmat3 ComputeEastNorthUpToEcef(const glm::dvec3& Ecef);

public:
  /**
   * Transforms the given point from Earth-Centered, Earth-Fixed (ECEF) into
   * Unreal relative world (relative to the floating origin).
   * @param Ecef The ECEF coordinate to transform
   * @param EcefToUeAbsoluteWorld The transform from ECEF to Unreal Absolute
   * World for a given georeference.
   * @param UeOriginLocation The location of the Unreal relative frame's
   * floating origin.
   */
  static glm::dvec3 TransformEcefToUnreal(
      const glm::dvec3& Ecef,
      const glm::dmat4& EcefToUeAbsoluteWorld,
      const glm::dvec3& UeOriginLocation);

  /**
   * Transforms the given point from Unreal relative world (relative to the
   * floating origin) to Earth-Centered, Earth-Fixed (ECEF).
   * @param Ue The Unreal coordinate in relative world space
   * @param UeAbsoluteWorldToEcef The transform from Unreal's absolute world
   * origin to ECEF
   * @param UeOriginLocation The location of the Unreal relative frame's
   * floating origin.
   */
  static glm::dvec3 TransformUnrealToEcef(
      const glm::dvec3& Ue,
      const glm::dmat4& UeAbsoluteWorldToEcef,
      const glm::dvec3& UeOriginLocation);
};
