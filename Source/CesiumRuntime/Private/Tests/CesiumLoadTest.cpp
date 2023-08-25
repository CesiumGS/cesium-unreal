// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#if WITH_EDITOR

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"

#include "Editor.h"
#include "Engine/World.h"
#include "EngineAnalytics.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"

#include "Cesium3DTileset.h"
#include "CesiumCameraManager.h"
#include "CesiumGeoreference.h"
#include "CesiumIonRasterOverlay.h"
#include "CesiumRuntime.h"
#include "CesiumSunSky.h"
#include "CesiumTestHelpers.h"
#include "GlobeAwareDefaultPawn.h"

//
// For debugging, it may help to create the scene in the editor
// After the test is run, you can play with their settings and even run PIE
//
#define CREATE_TEST_IN_EDITOR_WORLD 1

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCesiumLoadTest,
    "Cesium.LoadTest",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

struct LoadTestContext {
  UWorld* world;
  ACesiumGeoreference* georeference;
  AGlobeAwareDefaultPawn* pawn;
  std::vector<ACesium3DTileset*> tilesets;
};

bool neverBreak(LoadTestContext& context) { return false; }

bool breakWhenTilesetsLoaded(LoadTestContext& context) {
  std::vector<ACesium3DTileset*>::const_iterator it;
  for (it = context.tilesets.begin(); it != context.tilesets.end(); ++it) {
    ACesium3DTileset* tileset = *it;
    if (tileset->GetLoadProgress() != 100)
      return false;
  }
  return true;
}

bool tickWorldUntil(
    LoadTestContext& context,
    double time,
    std::function<bool(LoadTestContext&)> breakFunction) {
  const double minStepTime = 0.001; // Don't loop faster than 100 fps

  const double testStartMark = FPlatformTime::Seconds();
  const double testEndMark = testStartMark + time;
  double lastTimeMark = testStartMark;

  while (1) {
    double frameTimeMark = FPlatformTime::Seconds();

    if (frameTimeMark > testEndMark)
      return true;

    if (breakFunction(context))
      return false;

    double frameElapsedTime = frameTimeMark - lastTimeMark;

    if (frameElapsedTime < minStepTime) {
      double sleepTarget = minStepTime - frameElapsedTime;
      FPlatformProcess::Sleep(sleepTarget);
      continue;
    }

    // Let world tick at same rate as this loop
    context.world->Tick(ELevelTick::LEVELTICK_All, frameElapsedTime);

    // Derived from TimerManagerTests.cpp, TimerTest_TickWorld
    GFrameCounter++;

    lastTimeMark = frameTimeMark;
  };
}

void setupForGoogleTiles(LoadTestContext& context) {

  FVector targetOrigin(-122.083969, 37.424492, 142.859116);
  FString targetUrl(
      "https://tile.googleapis.com/v1/3dtiles/root.json?key=AIzaSyCnRPXWDIj1LuX6OWIweIqZFHHoXVgdYss");

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

void setupForDenver(LoadTestContext& context) {

  FVector targetOrigin(-104.988892, 39.743462, 1798.679443);
  FString ionToken(
      "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJqdGkiOiI2NmZhZTk4NS01MDFmLTRjODgtOTlkYy04NjIwODhiZWExOGYiLCJpZCI6MjU5LCJpYXQiOjE2ODg1MTI4ODd9.haoe5hsJyfHk1dQAHVK6N8dW_kfmtdbyuhlGwFdEHbM");

  context.georeference->SetGeoreferenceOriginLongitudeLatitudeHeight(
      targetOrigin);

  context.pawn->SetActorLocation(FVector(0, 0, 0));
  context.pawn->SetActorRotation(FRotator(-5.2, -149.4, 0));

  // Add Cesium World Terrain
  ACesium3DTileset* worldTerrainTileset =
      context.world->SpawnActor<ACesium3DTileset>();
  worldTerrainTileset->SetTilesetSource(ETilesetSource::FromCesiumIon);
  worldTerrainTileset->SetIonAssetID(1);
  worldTerrainTileset->SetIonAccessToken(ionToken);
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
  aerometrexTileset->SetIonAccessToken(ionToken);
  aerometrexTileset->SetMaximumScreenSpaceError(2.0);
  aerometrexTileset->SetActorLabel(TEXT("Aerometrex Denver"));

  context.tilesets.push_back(worldTerrainTileset);
  context.tilesets.push_back(aerometrexTileset);
}

void createCommonWorldObjects(LoadTestContext& context) {

#if CREATE_TEST_IN_EDITOR_WORLD
  context.world = FAutomationEditorCommonUtils::CreateNewMap();
#else
  context.world = UWorld::CreateWorld(EWorldType::Game, false);
  FWorldContext& worldContext =
      GEngine->CreateNewWorldContext(EWorldType::Game);
  worldContext.SetCurrentWorld(context.world);
#endif

  ACesiumCameraManager* cameraManager =
      ACesiumCameraManager::GetDefaultCameraManager(context.world);
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
}

bool FCesiumLoadTest::RunTest(const FString& Parameters) {

  //
  // Programmatically set up the world
  //
  LoadTestContext context;
  createCommonWorldObjects(context);

  // Configure location specific objects
  const size_t locationIndex = 1;
  switch (locationIndex) {
  case 0:
    setupForGoogleTiles(context);
    break;
  case 1:
    setupForDenver(context);
    break;
  default:
    break;
  }

  // Halt tileset updates and reset them
  std::vector<ACesium3DTileset*>::iterator it;
  for (it = context.tilesets.begin(); it != context.tilesets.end(); ++it) {
    ACesium3DTileset* tileset = *it;
    tileset->SuspendUpdate = true;
    tileset->RefreshTileset();
  }

  // Let world settle for 1 second
  tickWorldUntil(context, 1.0, neverBreak);

  // Turn updates back on
  for (it = context.tilesets.begin(); it != context.tilesets.end(); ++it) {
    ACesium3DTileset* tileset = *it;
    tileset->SuspendUpdate = false;
  }

  // Spin for a maximum of 5 seconds, or until tilesets finish loading
  bool timedOut = tickWorldUntil(context, 5.0, breakWhenTilesetsLoaded);

  if (FEngineAnalytics::IsAvailable()) {
    if (timedOut) {
      FEngineAnalytics::GetProvider().RecordEvent(
          TEXT("LoadTest.TimeoutEvent"));
      UE_LOG(LogCesium, Error, TEXT("Test timed out"));
    } else {
      FEngineAnalytics::GetProvider().RecordEvent(
          TEXT("LoadTest.CompletionEvent"));
      UE_LOG(LogCesium, Display, TEXT("Test completed"));
    }
  }

  // Cleanup
#if CREATE_TEST_IN_EDITOR_WORLD
  // Let all objects persist
#else
  GEngine->DestroyWorldContext(context.world);
  context.world->DestroyWorld(false);
#endif

  return !timedOut;
}

#endif
