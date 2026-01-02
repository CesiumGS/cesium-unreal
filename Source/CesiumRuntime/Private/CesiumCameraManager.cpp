// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "CesiumCameraManager.h"
#include "CesiumRuntime.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include <string>
#include <vector>

FName ACesiumCameraManager::DEFAULT_CAMERAMANAGER_TAG =
    FName("DEFAULT_CAMERAMANAGER");

/*static*/ ACesiumCameraManager* ACesiumCameraManager::GetDefaultCameraManager(
    const UObject* WorldContextObject) {
  // A null world context means a null return value (no camera manager
  // available)
  if (WorldContextObject == nullptr)
    return nullptr;
  UWorld* world = WorldContextObject->GetWorld();
  // This method can be called by actors even when opening the content browser.
  if (!IsValid(world)) {
    return nullptr;
  }
  UE_LOG(
      LogCesium,
      Verbose,
      TEXT("World name for GetDefaultCameraManager: %s"),
      *world->GetFullName());

  // Note: The actor iterator will be created with the
  // "EActorIteratorFlags::SkipPendingKill" flag,
  // meaning that we don't have to handle objects
  // that have been deleted. (This is the default,
  // but made explicit here)
  ACesiumCameraManager* pCameraManager = nullptr;
  EActorIteratorFlags flags = EActorIteratorFlags::OnlyActiveLevels |
                              EActorIteratorFlags::SkipPendingKill;
  for (TActorIterator<AActor> actorIterator(
           world,
           ACesiumCameraManager::StaticClass(),
           flags);
       actorIterator;
       ++actorIterator) {
    AActor* actor = *actorIterator;
    if (actor->GetLevel() == world->PersistentLevel &&
        actor->ActorHasTag(DEFAULT_CAMERAMANAGER_TAG)) {
      pCameraManager = Cast<ACesiumCameraManager>(actor);
      break;
    }
  }

  if (!pCameraManager) {
    UE_LOG(
        LogCesium,
        Verbose,
        TEXT("Creating default ACesiumCameraManager for actor %s"),
        *WorldContextObject->GetName());
    // Spawn georeference in the persistent level
    FActorSpawnParameters spawnParameters;
    spawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    spawnParameters.OverrideLevel = world->PersistentLevel;
    pCameraManager = world->SpawnActor<ACesiumCameraManager>(spawnParameters);
    // Null check so the editor doesn't crash when it makes arbitrary calls to
    // this function without a valid world context object.
    if (pCameraManager) {
      pCameraManager->Tags.Add(DEFAULT_CAMERAMANAGER_TAG);
    }
  } else {
    UE_LOG(
        LogCesium,
        Verbose,
        TEXT("Using existing ACesiumCameraManager %s for actor %s"),
        *pCameraManager->GetName(),
        *WorldContextObject->GetName());
  }
  return pCameraManager;
}

ACesiumCameraManager::ACesiumCameraManager() : AActor() {
#if WITH_EDITOR
  this->SetIsSpatiallyLoaded(false);
#endif
}

bool ACesiumCameraManager::ShouldTickIfViewportsOnly() const { return true; }

void ACesiumCameraManager::Tick(float DeltaTime) { Super::Tick(DeltaTime); }

int32 ACesiumCameraManager::AddCamera(UPARAM(ref) const FCesiumCamera& camera) {
  int32 cameraId = this->_currentCameraId++;
  this->cameras.Emplace(cameraId, camera);
  return cameraId;
}

bool ACesiumCameraManager::RemoveCamera(int32 cameraId) {
  int32 numRemovedPairs = this->cameras.Remove(cameraId);
  bool success = numRemovedPairs > 0;
  return success;
}

bool ACesiumCameraManager::UpdateCamera(
    int32 cameraId,
    UPARAM(ref) const FCesiumCamera& camera) {
  FCesiumCamera* pCurrentCamera = this->cameras.Find(cameraId);
  if (pCurrentCamera) {
    *pCurrentCamera = camera;
    return true;
  }

  return false;
}

const TMap<int32, FCesiumCamera>& ACesiumCameraManager::GetCameras() const {
  return this->cameras;
}
