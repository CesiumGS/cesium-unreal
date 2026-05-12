// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumActors.h"
#include "CesiumRuntime.h"
#include "Engine/World.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

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

bool CesiumActors::shouldValidateFlags(UObject* object) {
#if WITH_EDITOR
  // Only fixup flags in the editor, when not in play mode
  if (!IsValid(GEditor) || GEditor->IsPlaySessionInProgress())
    return false;

  // In addition, don't fix when loading from an asset file, which sets the
  // RF_ClassDefaultObject and RF_ArchetypeObject flags.
  if (object->HasAnyFlags(RF_ClassDefaultObject) ||
      object->HasAnyFlags(RF_ArchetypeObject))
    return false;
  else
    return true;
#else
  return false;
#endif
}

void CesiumActors::validatePublicFlag(UObject* object, const FString& label) {
  //
  // From an Epic Engine Developer...
  // RF_Public means that the object is an asset, so it should be only set for
  // worlds, textures, meshes, etc. This flags essentially says it's ok to
  // have a reference to public objects from other packages (with the
  // exception of worlds). Actors are not assets since they are part of a
  // level which is part of a world, etc. that's why they should not key the
  // public flag.
  //
  // In earlier versions of cesium-unreal, this flag may have been set
  //
  if (object->HasAnyFlags(RF_Public)) {
    UE_LOG(
        LogCesium,
        Display,
        TEXT("Clearing invalid RF_Public flag on %s"),
        *label);
    object->ClearFlags(RF_Public);
  }
}

void CesiumActors::validateActorFlags(AActor* actor) {
  FString label = FString("actor: ") + *actor->GetName();
  validatePublicFlag(actor, label);
}

void CesiumActors::validateActorComponentFlags(UActorComponent* component) {
  FString label = FString("actor component: ") + *component->GetName();
  validatePublicFlag(component, label);
}
