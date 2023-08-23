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
#define CREATE_TEST_IN_EDITOR_WORLD 0


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCesiumLoadTest,
    "Cesium.LoadTest",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

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

  // Create world objects
  ACesiumCameraManager* cameraManager =
      ACesiumCameraManager::GetDefaultCameraManager(world);
  ACesiumGeoreference* georeference =
      ACesiumGeoreference::GetDefaultGeoreference(world);
  ACesiumSunSky* sunSkyActor = world->SpawnActor<ACesiumSunSky>();
  ACesium3DTileset* tilesetActor = world->SpawnActor<ACesium3DTileset>();
  APlayerStart* playerStartActor = world->SpawnActor<APlayerStart>();

  FSoftObjectPath objectPath(
      TEXT("Class'/CesiumForUnreal/DynamicPawn.DynamicPawn_C'"));
  TSoftObjectPtr<UObject> DynamicPawn = TSoftObjectPtr<UObject>(objectPath);
  AGlobeAwareDefaultPawn* pawnActor = world->SpawnActor<AGlobeAwareDefaultPawn>(
      Cast<UClass>(DynamicPawn.LoadSynchronous()));

  TestNotNull("cameraManager is valid", cameraManager);
  TestNotNull("georeference is valid", georeference);
  TestNotNull("sunSkyActor is valid", sunSkyActor);
  TestNotNull("tilesetActor is valid", tilesetActor);
  TestNotNull("playerStartActor is valid", playerStartActor);
  TestNotNull("pawnActor is valid", pawnActor);

  // Configure similar to Google Tiles sample
  FVector targetOrigin(-122.083969, 37.424492, 142.859116);
  FString targetUrl(
      "https://tile.googleapis.com/v1/3dtiles/root.json?key=AIzaSyCnRPXWDIj1LuX6OWIweIqZFHHoXVgdYss");

  georeference->SetGeoreferenceOriginLongitudeLatitudeHeight(targetOrigin);
  tilesetActor->SetUrl(targetUrl);
  tilesetActor->SetTilesetSource(ETilesetSource::FromUrl);
  pawnActor->SetActorLocation(FVector(0, 0, 0));
  pawnActor->SetActorRotation(FRotator(-25, 95, 0));
  pawnActor->AutoPossessPlayer = EAutoReceiveInput::Player0;

#if CREATE_TEST_IN_EDITOR_WORLD
  // Let all objects persist
#else
  GEngine->DestroyWorldContext(world);
  world->DestroyWorld(false);
#endif

  return true;
}

#endif
