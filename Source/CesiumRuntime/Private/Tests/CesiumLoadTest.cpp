// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#if WITH_EDITOR

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"

#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"

#include "Cesium3DTileset.h"
#include "CesiumCameraManager.h"
#include "CesiumGeoreference.h"
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

void tickWorld(UWorld* world, double time) {
  const double minStepTime = 0.001; // Don't loop faster than 100 fps

  const double testStartMark = FPlatformTime::Seconds();
  const double testEndMark = testStartMark + time;
  double lastTimeMark = testStartMark;

  while (1) {
    double frameTimeMark = FPlatformTime::Seconds();

    if (frameTimeMark > testEndMark)
      break;

    double frameElapsedTime = frameTimeMark - lastTimeMark;

    if (frameElapsedTime < minStepTime) {
      double sleepTarget = minStepTime - frameElapsedTime;
      FPlatformProcess::Sleep(sleepTarget);
      continue;
    }

    // Let world tick at same rate as this loop
    world->Tick(ELevelTick::LEVELTICK_All, frameElapsedTime);

    // Derived from TimerManagerTests.cpp, TimerTest_TickWorld
    GFrameCounter++;

    lastTimeMark = frameTimeMark;
  };
}

void setupForGoogleTiles(UWorld* world) {
  ACesiumCameraManager* cameraManager =
      ACesiumCameraManager::GetDefaultCameraManager(world);
  ACesiumGeoreference* georeference =
      ACesiumGeoreference::GetDefaultGeoreference(world);
  ACesiumSunSky* sunSky = world->SpawnActor<ACesiumSunSky>();
  ACesium3DTileset* tileset = world->SpawnActor<ACesium3DTileset>();
  APlayerStart* playerStart = world->SpawnActor<APlayerStart>();

  FSoftObjectPath objectPath(
      TEXT("Class'/CesiumForUnreal/DynamicPawn.DynamicPawn_C'"));
  TSoftObjectPtr<UObject> DynamicPawn = TSoftObjectPtr<UObject>(objectPath);
  AGlobeAwareDefaultPawn* pawn = world->SpawnActor<AGlobeAwareDefaultPawn>(
      Cast<UClass>(DynamicPawn.LoadSynchronous()));
  pawn->AutoPossessPlayer = EAutoReceiveInput::Player0;

  FVector targetOrigin(-122.083969, 37.424492, 142.859116);
  FString targetUrl(
      "https://tile.googleapis.com/v1/3dtiles/root.json?key=AIzaSyCnRPXWDIj1LuX6OWIweIqZFHHoXVgdYss");

  georeference->SetGeoreferenceOriginLongitudeLatitudeHeight(targetOrigin);
  tileset->SetUrl(targetUrl);
  tileset->SetTilesetSource(ETilesetSource::FromUrl);
  pawn->SetActorLocation(FVector(0, 0, 0));
  pawn->SetActorRotation(FRotator(-25, 95, 0));
}

void setupForDenver(UWorld* world) {
  ACesiumCameraManager* cameraManager =
      ACesiumCameraManager::GetDefaultCameraManager(world);
  ACesiumGeoreference* georeference =
      ACesiumGeoreference::GetDefaultGeoreference(world);
  ACesiumSunSky* sunSky = world->SpawnActor<ACesiumSunSky>();
  ACesium3DTileset* worldTileset = world->SpawnActor<ACesium3DTileset>();
  ACesium3DTileset* denverTileset = world->SpawnActor<ACesium3DTileset>();
  APlayerStart* playerStart = world->SpawnActor<APlayerStart>();

  FSoftObjectPath objectPath(
      TEXT("Class'/CesiumForUnreal/DynamicPawn.DynamicPawn_C'"));
  TSoftObjectPtr<UObject> DynamicPawn = TSoftObjectPtr<UObject>(objectPath);
  AGlobeAwareDefaultPawn* pawn = world->SpawnActor<AGlobeAwareDefaultPawn>(
      Cast<UClass>(DynamicPawn.LoadSynchronous()));
  pawn->AutoPossessPlayer = EAutoReceiveInput::Player0;

  FVector targetOrigin(-104.988892, 39.743462, 1798.679443);
  FString ionToken(
      "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJqdGkiOiI2NmZhZTk4NS01MDFmLTRjODgtOTlkYy04NjIwODhiZWExOGYiLCJpZCI6MjU5LCJpYXQiOjE2ODg1MTI4ODd9.haoe5hsJyfHk1dQAHVK6N8dW_kfmtdbyuhlGwFdEHbM");

  georeference->SetGeoreferenceOriginLongitudeLatitudeHeight(targetOrigin);
  worldTileset->SetTilesetSource(ETilesetSource::FromCesiumIon);
  worldTileset->SetIonAssetID(1);
  worldTileset->SetIonAccessToken(ionToken);

  denverTileset->SetTilesetSource(ETilesetSource::FromCesiumIon);
  denverTileset->SetIonAssetID(354307);
  denverTileset->SetIonAccessToken(ionToken);
  denverTileset->SetMaximumScreenSpaceError(2.0);

  pawn->SetActorLocation(FVector(0, 0, 0));
  pawn->SetActorRotation(FRotator(-5.2, -149.4, 0));
}

bool FCesiumLoadTest::RunTest(const FString& Parameters) {

  //
  // Programmatically set up the world
  //

#if CREATE_TEST_IN_EDITOR_WORLD
  UWorld* world = FAutomationEditorCommonUtils::CreateNewMap();
#else
  UWorld* world = UWorld::CreateWorld(EWorldType::Game, false);
  FWorldContext& context = GEngine->CreateNewWorldContext(EWorldType::Game);
  context.SetCurrentWorld(world);
#endif
  TestNotNull("world is valid", world);

  // Configure similar to Google Tiles sample
  // setupForGoogleTiles(world);
  setupForDenver(world);

  world->BeginPlay();

  // Spin for 5 seconds, letting our game objects tick
  const double testMaxTime = 5.0;
  tickWorld(world, testMaxTime);

  // Cleanup
#if CREATE_TEST_IN_EDITOR_WORLD
  // Let all objects persist
#else
  GEngine->DestroyWorldContext(world);
  world->DestroyWorld(false);
#endif

  return true;
}

#endif
