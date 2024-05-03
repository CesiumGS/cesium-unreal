// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#if WITH_EDITOR

#include "CesiumSceneGeneration.h"

#include "Tests/AutomationEditorCommon.h"

#include "GameFramework/PlayerStart.h"
#include "LevelEditorViewport.h"

#include "CesiumAsync/ICacheDatabase.h"
#include "CesiumRuntime.h"
#include "Cesium3DTileset.h"
#include "CesiumGeoreference.h"
#include "CesiumLoadTestCore.h"
#include "CesiumSunSky.h"
#include "GlobeAwareDefaultPawn.h"

#include "CesiumTestHelpers.h"

namespace Cesium {

FString SceneGenerationContext::testIonToken(
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJqdGkiOiJlMzdlODZmOS0wMzRmLTRmOTUtYWFkMC1iMTdhZGZjMjJmM2EiLCJpZCI6MjU5LCJpYXQiOjE3MDkyNTQxNjF9.osAkBsXPBmrGgcN0jr9RTJnwQfid4tJb3XO39tAZX2s");

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

inline void GoogleTilesTestSetup::setupRefreshTilesets(
    SceneGenerationContext& context,
    TestPass::TestingParameter parameter) {
  context.refreshTilesets();
}

inline void GoogleTilesTestSetup::setupClearCache(
    SceneGenerationContext& context,
    TestPass::TestingParameter parameter) {
  std::shared_ptr<CesiumAsync::ICacheDatabase> pCacheDatabase =
      getCacheDatabase();
  pCacheDatabase->clearAll();
}

inline void GoogleTilesTestSetup::setupForLocation(
    SceneGenerationContext& context,
    const FVector& location,
    const FRotator& rotation,
    const FString& name) {
  context.setCommonProperties(location, FVector::ZeroVector, rotation, 60.0f);

  ACesium3DTileset* tileset = context.world->SpawnActor<ACesium3DTileset>();
  tileset->SetTilesetSource(ETilesetSource::FromCesiumIon);
  tileset->SetIonAssetID(2275207);
  tileset->SetIonAccessToken(SceneGenerationContext::testIonToken);
  tileset->SetActorLabel(name);
  context.tilesets.push_back(tileset);
}

inline void
GoogleTilesTestSetup::setupForPompidou(SceneGenerationContext& context) {
  setupForLocation(
      context,
      FVector(2.352200, 48.860600, 200),
      FRotator(-20.0, -90.0, 0.0),
      TEXT("Center Pompidou, Paris, France"));

  context.sunSky->TimeZone = 2.0f;
  context.sunSky->UpdateSun();
}

inline void
GoogleTilesTestSetup::setupForChrysler(SceneGenerationContext& context) {
  setupForLocation(
      context,
      FVector(-73.9752624659, 40.74697185903, 307.38),
      FRotator(-15.0, -90.0, 0.0),
      TEXT("Chrysler Building, NYC"));

  context.sunSky->TimeZone = -4.0f;
  context.sunSky->UpdateSun();
}

inline void
GoogleTilesTestSetup::setupForGuggenheim(SceneGenerationContext& context) {
  setupForLocation(
      context,
      FVector(-2.937, 43.2685, 150),
      FRotator(-15.0, 0.0, 0.0),
      TEXT("Guggenheim Museum, Bilbao, Spain"));

  context.sunSky->TimeZone = 2.0f;
  context.sunSky->UpdateSun();
}

inline void
GoogleTilesTestSetup::setupForDeathValley(SceneGenerationContext& context) {
  setupForLocation(
      context,
      FVector(-116.812278, 36.42, 300),
      FRotator(0, 0.0, 0.0),
      TEXT("Zabriskie Point, Death Valley National Park, California"));

  context.sunSky->TimeZone = -7.0f;
  context.sunSky->UpdateSun();
}

inline void
GoogleTilesTestSetup::setupForTokyo(SceneGenerationContext& context) {
  setupForLocation(
      context,
      FVector(139.7563178458, 35.652798383944, 525.62),
      FRotator(-15, -150, 0.0),
      TEXT("Tokyo Tower, Tokyo, Japan"));

  context.sunSky->TimeZone = 9.0f;
  context.sunSky->UpdateSun();
}

inline void
GoogleTilesTestSetup::setupForGoogleplex(SceneGenerationContext& context) {
  setupForLocation(
      context,
      FVector(-122.083969, 37.424492, 142.859116),
      FRotator(-25, 95, 0),
      TEXT("Google Photorealistic 3D Tiles"));
}

} // namespace Cesium

#endif // #if WITH_EDITOR
