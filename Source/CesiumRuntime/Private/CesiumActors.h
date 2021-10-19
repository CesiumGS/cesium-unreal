// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#pragma once

#include "GameFramework/Actor.h"
#include <glm/glm.hpp>

/**
 * @brief Utility functions related to Unreal actors
 */
class CesiumActors {
public:
  /**
   * @brief Returns the origin location of the world that the given
   * actor is contained in.
   *
   * If the given actor is not valid or not contained in a world,
   * then a warning is printed and (0,0,0,0) is returned.
   *
   * @param actor The actor
   * @return The world origin
   */
  static glm::dvec4 getWorldOrigin4D(const AActor* actor);
};
