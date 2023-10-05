// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#if WITH_EDITOR

#include "CesiumSceneGeneration.h"

#include "Tests/AutomationEditorCommon.h"

#include "GameFramework/PlayerStart.h"
#include "LevelEditorViewport.h"

#include "Cesium3DTileset.h"
#include "CesiumGeoreference.h"
#include "CesiumIonRasterOverlay.h"
#include "CesiumSunSky.h"
#include "GlobeAwareDefaultPawn.h"

#include "CesiumTestHelpers.h"

namespace Cesium {

FString SceneGenerationContext::testIonToken(
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJqdGkiOiI3NjU3OGE4Zi0xOGM4LTQ4NjYtODc4ZS02YWNkMDZmY2Y1M2YiLCJpZCI6MjU5LCJpYXQiOjE2OTA4Nzg3MjB9.uxePYJL59S4pG5aqJHb9goikVSO-Px6xA7kZH8oM1eM");
FString SceneGenerationContext::testGoogleUrl(
    "https://tile.googleapis.com/v1/3dtiles/root.json?key=AIzaSyCnRPXWDIj1LuX6OWIweIqZFHHoXVgdYss");

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

  ACesiumSunSky* sunSky = context.world->SpawnActor<ACesiumSunSky>();
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

void setupForGoogleplex(SceneGenerationContext& context) {

  context.setCommonProperties(
      FVector(-122.083969, 37.424492, 142.859116),
      FVector(0, 0, 0),
      FRotator(-25, 95, 0),
      90.0f);

  ACesium3DTileset* tileset = context.world->SpawnActor<ACesium3DTileset>();
  tileset->SetUrl(SceneGenerationContext::testGoogleUrl);
  tileset->SetTilesetSource(ETilesetSource::FromUrl);
  tileset->SetActorLabel(TEXT("Google Photorealistic 3D Tiles"));

  context.tilesets.push_back(tileset);
}

void setupForDenver(SceneGenerationContext& context) {

  context.setCommonProperties(
      FVector(-104.988892, 39.743462, 1798.679443),
      FVector(0, 0, 0),
      FRotator(-5.2, -149.4, 0),
      90.0f);

  // Add Cesium World Terrain
  ACesium3DTileset* worldTerrainTileset =
      context.world->SpawnActor<ACesium3DTileset>();
  worldTerrainTileset->SetTilesetSource(ETilesetSource::FromCesiumIon);
  worldTerrainTileset->SetIonAssetID(1);
  worldTerrainTileset->SetIonAccessToken(SceneGenerationContext::testIonToken);
  worldTerrainTileset->SetActorLabel(TEXT("Cesium World Terrain"));

  // Bing Maps Aerial overlay
  UCesiumIonRasterOverlay* pOverlay = NewObject<UCesiumIonRasterOverlay>(
      worldTerrainTileset,
      FName("Bing Maps Aerial"),
      RF_Transactional);
  pOverlay->MaterialLayerKey = TEXT("Overlay0");
  pOverlay->IonAssetID = 2;
  pOverlay->SetActive(true);
  pOverlay->OnComponentCreated();
  worldTerrainTileset->AddInstanceComponent(pOverlay);

  // Aerometrex Denver
  ACesium3DTileset* aerometrexTileset =
      context.world->SpawnActor<ACesium3DTileset>();
  aerometrexTileset->SetTilesetSource(ETilesetSource::FromCesiumIon);
  aerometrexTileset->SetIonAssetID(354307);
  aerometrexTileset->SetIonAccessToken(SceneGenerationContext::testIonToken);
  aerometrexTileset->SetMaximumScreenSpaceError(2.0);
  aerometrexTileset->SetActorLabel(TEXT("Aerometrex Denver"));

  context.tilesets.push_back(worldTerrainTileset);
  context.tilesets.push_back(aerometrexTileset);
}

void setupForMontrealPointCloud(SceneGenerationContext& context) {

  context.setCommonProperties(
      FVector(-73.616526, 45.57335, 95.048859),
      FVector(0, 0, 0),
      FRotator(-90.0, 0.0, 0.0),
      90.0f);

  ACesium3DTileset* montrealTileset =
      context.world->SpawnActor<ACesium3DTileset>();
  montrealTileset->SetTilesetSource(ETilesetSource::FromCesiumIon);
  montrealTileset->SetIonAssetID(28945);
  montrealTileset->SetIonAccessToken(SceneGenerationContext::testIonToken);
  montrealTileset->SetMaximumScreenSpaceError(16.0);
  montrealTileset->SetActorLabel(TEXT("Montreal Point Cloud"));

  context.tilesets.push_back(montrealTileset);
}

} // namespace Cesium

#endif // #if WITH_EDITOR
