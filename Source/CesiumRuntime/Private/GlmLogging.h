// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include <string>
#include <glm/glm.hpp>

/**
 * @brief Utility functions for logging GLM data in Unreal
 */
class GlmLogging {
public:
  /**
   * Print the given vector as a verbose `LogCesium`
   * message, with unspecified formatting.
   *
   * @param name The name that will be part of the message
   * @param vector The vector
   */
  static void logVector(const std::string& name, const glm::dvec3& vector);

  /**
   * Print the given matrix as a verbose `LogCesium`
   * message, with unspecified formatting.
   *
   * @param name The name that will be part of the message
   * @param matrix The matrix
   */
  static void logMatrix(const std::string& name, const glm::dmat4& matrix);
};
