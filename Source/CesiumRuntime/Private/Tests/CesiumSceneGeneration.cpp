// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#if WITH_EDITOR

#include "CesiumSceneGeneration.h"

#include "Tests/AutomationEditorCommon.h"

#include "GameFramework/PlayerStart.h"
#include "LevelEditorViewport.h"

#include "Cesium3DTileset.h"
#include "CesiumAsync/ICacheDatabase.h"
#include "CesiumGeoreference.h"
#include "CesiumLoadTestCore.h"
#include "CesiumRuntime.h"
#include "CesiumSunSky.h"
#include "GlobeAwareDefaultPawn.h"

#include "CesiumTestHelpers.h"

namespace Cesium {

FString SceneGenerationContext::testIonToken(
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJqdGkiOiIyOGEwZmZhZS03N2U3LTQ3YzMtYWRhYy05OGY0MDIxY2ZkMWEiLCJpZCI6MjU5LCJpYXQiOjE3MTczODI3OTd9.xo6dUz0EilPaeagaXnmRKGLyVL4lTk3P99u1_1N78js");

void SceneGenerationContext::setCommonProperties(
    const FVector& origin,
    const FVector& position,
    const FRotator& rotation,
    float fieldOfView) {
  startPosition = position;
  startRotation = rotation;
  startFieldOfView = fieldOfView;

  georeference->SetOriginLongitudeLatitudeHeight(origin);

  pawn->SetActorLocation(startPosition);
  pawn->SetActorRotation(startRotation);

  TInlineComponentArray<UCameraComponent*> cameras;
  pawn->GetComponents<UCameraComponent>(cameras);
  for (UCameraComponent* cameraComponent : cameras)
    cameraComponent->SetFieldOfView(startFieldOfView);
}

void SceneGenerationContext::refreshTilesets() {
  std::vector<ACesium3DTileset*>::iterator it;
  for (it = tilesets.begin(); it != tilesets.end(); ++it)
    (*it)->RefreshTileset();
}

void SceneGenerationContext::setSuspendUpdate(bool suspend) {
  std::vector<ACesium3DTileset*>::iterator it;
  for (it = tilesets.begin(); it != tilesets.end(); ++it)
    (*it)->SuspendUpdate = suspend;
}

void SceneGenerationContext::setMaximumSimultaneousTileLoads(int value) {
  std::vector<ACesium3DTileset*>::iterator it;
  for (it = tilesets.begin(); it != tilesets.end(); ++it)
    (*it)->MaximumSimultaneousTileLoads = value;
}

bool SceneGenerationContext::areTilesetsDoneLoading() {
  if (tilesets.empty())
    return false;

  std::vector<ACesium3DTileset*>::const_iterator it;
  for (it = tilesets.begin(); it != tilesets.end(); ++it) {
    ACesium3DTileset* tileset = *it;

    int progress = (int)tileset->GetLoadProgress();
    if (progress != 100) {
      // We aren't done
      return false;
    }
  }
  return true;
}

void SceneGenerationContext::trackForPlay() {
  CesiumTestHelpers::trackForPlay(sunSky);
  CesiumTestHelpers::trackForPlay(georeference);
  CesiumTestHelpers::trackForPlay(pawn);

  std::vector<ACesium3DTileset*>::iterator it;
  for (it = tilesets.begin(); it != tilesets.end(); ++it) {
    ACesium3DTileset* tileset = *it;
    CesiumTestHelpers::trackForPlay(tileset);
  }
}

void SceneGenerationContext::initForPlay(
    SceneGenerationContext& creationContext) {
  world = GEditor->PlayWorld;
  sunSky = CesiumTestHelpers::findInPlay(creationContext.sunSky);
  georeference = CesiumTestHelpers::findInPlay(creationContext.georeference);
  pawn = CesiumTestHelpers::findInPlay(creationContext.pawn);

  startPosition = creationContext.startPosition;
  startRotation = creationContext.startRotation;
  startFieldOfView = creationContext.startFieldOfView;

  tilesets.clear();

  std::vector<ACesium3DTileset*>& creationTilesets = creationContext.tilesets;
  std::vector<ACesium3DTileset*>::iterator it;
  for (it = creationTilesets.begin(); it != creationTilesets.end(); ++it) {
    ACesium3DTileset* creationTileset = *it;
    ACesium3DTileset* tileset = CesiumTestHelpers::findInPlay(creationTileset);
    tilesets.push_back(tileset);
  }
}

void SceneGenerationContext::syncWorldCamera() {
  assert(GEditor);

  if (GEditor->IsPlayingSessionInEditor()) {
    // If in PIE, set the player
    assert(world->GetNumPlayerControllers() == 1);

    APlayerController* controller = world->GetFirstPlayerController();
    assert(controller);

    controller->ClientSetLocation(startPosition, startRotation);

    APlayerCameraManager* cameraManager = controller->PlayerCameraManager;
    assert(cameraManager);

    cameraManager->SetFOV(startFieldOfView);
  } else {
    // If editing, set any viewports
    for (FLevelEditorViewportClient* ViewportClient :
         GEditor->GetLevelViewportClients()) {
      if (ViewportClient == NULL)
        continue;
      ViewportClient->SetViewLocation(startPosition);
      ViewportClient->SetViewRotation(startRotation);
      if (ViewportClient->ViewportType == LVT_Perspective)
        ViewportClient->ViewFOV = startFieldOfView;
      ViewportClient->Invalidate();
    }
  }
}

void createCommonWorldObjects(SceneGenerationContext& context) {

  context.world = FAutomationEditorCommonUtils::CreateNewMap();

  context.sunSky = context.world->SpawnActor<ACesiumSunSky>();

  APlayerStart* playerStart = context.world->SpawnActor<APlayerStart>();

  FSoftObjectPath objectPath(
      TEXT("Class'/CesiumForUnreal/DynamicPawn.DynamicPawn_C'"));
  TSoftObjectPtr<UObject> DynamicPawn = TSoftObjectPtr<UObject>(objectPath);

  context.georeference =
      ACesiumGeoreference::GetDefaultGeoreference(context.world);
  context.pawn = context.world->SpawnActor<AGlobeAwareDefaultPawn>(
      Cast<UClass>(DynamicPawn.LoadSynchronous()));

  context.pawn->AutoPossessPlayer = EAutoReceiveInput::Player0;

  AWorldSettings* pWorldSettings = context.world->GetWorldSettings();
  if (pWorldSettings)
    pWorldSettings->bEnableWorldBoundsChecks = false;
}

} // namespace Cesium

#endif // #if WITH_EDITOR
