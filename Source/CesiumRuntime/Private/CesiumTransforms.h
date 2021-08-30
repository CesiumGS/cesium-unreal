// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include <glm/mat4x4.hpp>

/**
 * Constants useful for transformation between Cesium and Unreal Engine
 * coordinate systems.
 */
class CesiumTransforms {
public:
  /**
   * The constant to multiply to transform meters to centimeters (100.0).
   */
  static const double metersToCentimeters;

  /**
   * The constant to multiply to transform centimeters to meters (0.01).
   */
  static const double centimetersToMeters;

  /**
   * A matrix to scale Cesium's meters up to Unreal's centimeters.
   */
  static const glm::dmat4x4 scaleToUnrealWorld;

  /**
   * A matrix to scale down Unreal's centimeters into Cesium's meters.
   */
  static const glm::dmat4x4 scaleToCesium;

  /**
   * A matrix to transform Cesium's right-handed, Z-up coordinate system to
   * Unreal's left-handed, Z-up coordinate system by inverting the Y coordinate.
   * This same transformation can also go the other way.
   */
  static const glm::dmat4x4 unrealToOrFromCesium;
};
