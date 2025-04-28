// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#if WITH_EDITOR

#include "CesiumSceneGeneration.h"

#include "Tests/AutomationEditorCommon.h"

#include "GameFramework/PlayerStart.h"
#include "LevelEditorViewport.h"

#include "Cesium3DTileset.h"
#include "CesiumGeoreference.h"
#include "CesiumSunSky.h"
#include "GlobeAwareDefaultPawn.h"

#include "CesiumTestHelpers.h"

namespace Cesium {

FString SceneGenerationContext::testIonToken(
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJqdGkiOiI0N2E0OGRlNS1kYmZiLTQzYjctODBkOC0zOGYyOGZkZDAwNDciLCJpZCI6MjU5LCJpYXQiOjE3NDM0NzkyNjh9.LHKgeok4hnqfz2m1UwWaX0YCkcyjIHeCj49KpW_7mOU");

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

    float progress = tileset->GetLoadProgress();
    UE_LOG(
        LogCesium,
        Display,
        TEXT("Tileset %s Percent Loaded %f Suspended %d"),
        *tileset->GetName(),
        progress,
        (int)tileset->SuspendUpdate);
    if (progress < 100.0f) {
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
