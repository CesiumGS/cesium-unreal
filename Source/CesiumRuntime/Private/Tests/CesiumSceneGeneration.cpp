// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#include "CesiumSceneGeneration.h"

#if WITH_EDITOR

#include "Tests/AutomationEditorCommon.h"

#include "GameFramework/PlayerStart.h"

#include "Cesium3DTileset.h"
#include "CesiumCameraManager.h"
#include "CesiumGeoreference.h"
#include "CesiumIonRasterOverlay.h"
#include "CesiumSunSky.h"
#include "GlobeAwareDefaultPawn.h"

#include "CesiumTestHelpers.h"

namespace Cesium {

FString SceneGenerationContext::testIonToken(
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJqdGkiOiI3NjU3OGE4Zi0xOGM4LTQ4NjYtODc4ZS02YWNkMDZmY2Y1M2YiLCJpZCI6MjU5LCJpYXQiOjE2OTA4Nzg3MjB9.uxePYJL59S4pG5aqJHb9goikVSO-Px6xA7kZH8oM1eM");

void SceneGenerationContext::setCamera(const FCesiumCamera& camera) {
  // Take over first camera, or add if it doesn't exist
  const TMap<int32, FCesiumCamera> cameras = cameraManager->GetCameras();
  if (cameras.IsEmpty()) {
    cameraManager->AddCamera(camera);
  } else {
    cameraManager->UpdateCamera(0, camera);
  }
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
  CesiumTestHelpers::trackForPlay(cameraManager);
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
  cameraManager = CesiumTestHelpers::findInPlay(creationContext.cameraManager);
  pawn = CesiumTestHelpers::findInPlay(creationContext.pawn);

  tilesets.clear();

  std::vector<ACesium3DTileset*>& creationTilesets = creationContext.tilesets;
  std::vector<ACesium3DTileset*>::iterator it;
  for (it = creationTilesets.begin(); it != creationTilesets.end(); ++it) {
    ACesium3DTileset* creationTileset = *it;
    ACesium3DTileset* tileset = CesiumTestHelpers::findInPlay(creationTileset);
    tilesets.push_back(tileset);
  }
}

void createCommonWorldObjects(SceneGenerationContext& context) {

  context.world = FAutomationEditorCommonUtils::CreateNewMap();

  ACesiumSunSky* sunSky = context.world->SpawnActor<ACesiumSunSky>();
  APlayerStart* playerStart = context.world->SpawnActor<APlayerStart>();

  context.cameraManager =
      ACesiumCameraManager::GetDefaultCameraManager(context.world);

  FSoftObjectPath objectPath(
      TEXT("Class'/CesiumForUnreal/DynamicPawn.DynamicPawn_C'"));
  TSoftObjectPtr<UObject> DynamicPawn = TSoftObjectPtr<UObject>(objectPath);

  context.georeference =
      ACesiumGeoreference::GetDefaultGeoreference(context.world);
  context.pawn = context.world->SpawnActor<AGlobeAwareDefaultPawn>(
      Cast<UClass>(DynamicPawn.LoadSynchronous()));

  context.pawn->AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void setupForGoogleTiles(SceneGenerationContext& context) {

  FVector targetOrigin(-122.083969, 37.424492, 142.859116);
  FString targetUrl(
      "https://tile.googleapis.com/v1/3dtiles/root.json?key=AIzaSyCnRPXWDIj1LuX6OWIweIqZFHHoXVgdYss");

  FCesiumCamera camera;
  camera.ViewportSize = FVector2D(1024, 768);
  camera.Location = FVector(0, 0, 0);
  camera.Rotation = FRotator(-25, 95, 0);
  camera.FieldOfViewDegrees = 90;
  context.setCamera(camera);

  context.georeference->SetGeoreferenceOriginLongitudeLatitudeHeight(
      targetOrigin);

  context.pawn->SetActorLocation(FVector(0, 0, 0));
  context.pawn->SetActorRotation(FRotator(-25, 95, 0));

  ACesium3DTileset* tileset = context.world->SpawnActor<ACesium3DTileset>();
  tileset->SetUrl(targetUrl);
  tileset->SetTilesetSource(ETilesetSource::FromUrl);
  tileset->SetActorLabel(TEXT("Google Photorealistic 3D Tiles"));

  context.tilesets.push_back(tileset);
}

void setupForDenver(SceneGenerationContext& context) {

  FVector targetOrigin(-104.988892, 39.743462, 1798.679443);

  FCesiumCamera camera;
  camera.ViewportSize = FVector2D(1024, 768);
  camera.Location = FVector(0, 0, 0);
  camera.Rotation = FRotator(-5.2, -149.4, 0);
  camera.FieldOfViewDegrees = 90;
  context.setCamera(camera);

  context.georeference->SetGeoreferenceOriginLongitudeLatitudeHeight(
      targetOrigin);

  context.pawn->SetActorLocation(FVector(0, 0, 0));
  context.pawn->SetActorRotation(FRotator(-5.2, -149.4, 0));

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
  FVector targetOrigin(-73.616526, 45.57335, 95.048859);

  FCesiumCamera camera;
  camera.ViewportSize = FVector2D(1024, 768);
  camera.Location = FVector(0, 0, 0);
  camera.Rotation = FRotator(-90.0, 0.0, 0.0);
  camera.FieldOfViewDegrees = 90;
  context.setCamera(camera);

  context.georeference->SetGeoreferenceOriginLongitudeLatitudeHeight(
      targetOrigin);

  context.pawn->SetActorLocation(FVector(0, 0, 0));
  context.pawn->SetActorRotation(FRotator(-90.0, 0.0, 0.0));

  // Montreal Point Cloud
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
