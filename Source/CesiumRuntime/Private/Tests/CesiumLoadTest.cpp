// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#if WITH_EDITOR

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"

#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"

#include "Cesium3DTileset.h"
#include "CesiumCameraManager.h"
#include "CesiumGeoreference.h"
#include "CesiumRuntime.h"
#include "CesiumSunSky.h"
#include "CesiumTestHelpers.h"
#include "GlobeAwareDefaultPawn.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCesiumLoadTest,
    "Cesium.LoadTest",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

bool FCesiumLoadTest::RunTest(const FString& Parameters) {

  //
  // Programmatically set up the world
  //

  UWorld* world = CesiumTestHelpers::getGlobalWorldContext();
  ULevel* level = world->GetCurrentLevel();

  // Create a camera
  ACesiumCameraManager* cameraManager =
      ACesiumCameraManager::GetDefaultCameraManager(world);
  TestNotNull("Camera manager pointer is valid", cameraManager);

  // Create world objects
  ACesiumGeoreference* georeference =
      ACesiumGeoreference::GetDefaultGeoreference(world);
  ACesiumSunSky* sunSkyActor = world->SpawnActor<ACesiumSunSky>();
  ACesium3DTileset* tilesetActor = world->SpawnActor<ACesium3DTileset>();

  FSoftObjectPath objectPath(
      TEXT("Class'/CesiumForUnreal/DynamicPawn.DynamicPawn_C'"));
  TSoftObjectPtr<UObject> DynamicPawn = TSoftObjectPtr<UObject>(objectPath);
  AGlobeAwareDefaultPawn* pawnActor = world->SpawnActor<AGlobeAwareDefaultPawn>(
      Cast<UClass>(DynamicPawn.LoadSynchronous()));

  TestNotNull("Georeference pointer is valid", georeference);
  TestNotNull("SunSky actor pointer is valid", sunSkyActor);
  TestNotNull("Tileset actor pointer is valid", tilesetActor);
  TestNotNull("Dynamic pawn actor pointer is valid", pawnActor);

  // Configure similar to Google Tiles sample
  FVector targetOrigin(-122.083969, 37.424492, 142.859116);
  FString targetUrl(
      "https://tile.googleapis.com/v1/3dtiles/root.json?key=AIzaSyCnRPXWDIj1LuX6OWIweIqZFHHoXVgdYss");

  georeference->SetGeoreferenceOriginLongitudeLatitudeHeight(targetOrigin);
  tilesetActor->SetUrl(targetUrl);
  tilesetActor->SetTilesetSource(ETilesetSource::FromUrl);
  pawnActor->SetActorRotation(FRotator(-25, 95, 0));

  return true;
}

#endif
