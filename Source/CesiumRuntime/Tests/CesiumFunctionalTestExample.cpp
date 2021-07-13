// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "Editor.h"
#include "EditorModeManager.h"
#include "Engine.h"
#include "Misc/AutomationTest.h"

#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"

#if WITH_DEV_AUTOMATION_TESTS

/*
 * A trivial example of a functional test that just creates
 * a new world and an actor.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCesiumFunctionalTestExample,
    "Cesium.Examples.FunctionalTestExample",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FCesiumFunctionalTestExample::RunTest(const FString& Parameters) {
  const FTransform IdentityTransform(FVector(0.0f, 0.0f, 0.0f));

  UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
  ULevel* currentLevel = World->GetCurrentLevel();

  UClass* sunSkyClass = LoadClass<AActor>(
      nullptr,
      TEXT("/CesiumForUnreal/CesiumSunSky.CesiumSunSky_C"));
  AActor* sunSky =
      GEditor->AddActor(currentLevel, sunSkyClass, IdentityTransform);
  TestNotNull(TEXT("A CesiumSunSky instance could be created"), sunSky);

  FAutomationEditorCommonUtils::CreateNewMap();
  // GLevelEditorModeTools().MapChangeNotify();

  return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
