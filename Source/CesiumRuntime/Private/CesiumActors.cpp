// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "CesiumActors.h"
#include "CesiumRuntime.h"
#include "Engine/World.h"

#include <glm/glm.hpp>

glm::dvec4 CesiumActors::getWorldOrigin4D(const AActor* actor) {
  if (!IsValid(actor)) {
    UE_LOG(LogCesium, Warning, TEXT("The actor is not valid"));
    return glm::dvec4();
  }
  const UWorld* world = actor->GetWorld();
  if (!IsValid(world)) {
    UE_LOG(
        LogCesium,
        Warning,
        TEXT("The actor %s is not spawned in a level"),
        *actor->GetName());
    return glm::dvec4();
  }
  const FIntVector& originLocation = world->OriginLocation;
  return glm::dvec4(originLocation.X, originLocation.Y, originLocation.Z, 1.0);
}
