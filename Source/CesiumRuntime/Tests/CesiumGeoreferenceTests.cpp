// Copyright 2020-2021 CesiumGS, Inc. and Contributors

#include "Editor.h"
#include "EditorModeManager.h"
#include "Engine.h"
#include "Misc/AutomationTest.h"

#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"

#include "CesiumTestClasses.h"

#include <Cesium3DTileset.h>
#include <GlobeAwareDefaultPawn.h>

#if WITH_DEV_AUTOMATION_TESTS

//============================================================================

/*
 * A test for https://github.com/CesiumGS/cesium-unreal/issues/242 :
 *
 * - Creates a Tileset and adds it to the level
 * - Expects the default Georererence to be created and valid
 * - Deletes the Tileset and the Georeference
 * - Expects the deleted objects to be invalid
 * - Creates a second Tileset
 * - Expects a new default Georeference to be created and valid
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCesiumGeoreferenceAutoCreation,
    "Cesium.Georeference.AutoCreation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FCesiumGeoreferenceAutoCreation::RunTest(const FString& Parameters) {
  const FTransform IdentityTransform(FVector(0.0f, 0.0f, 0.0f));

  UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
  ULevel* currentLevel = World->GetCurrentLevel();

  // TODO Each Error message will cause the test to fail - OUCH.
  // We can work around that for now, by expecting the error
  // message, but we certainly do NOT want to rely on certain
  // error messages from cesium-native!
  int32 occurrences = 0; // Means one or more times
  AddExpectedError(
      TEXT(".*Received status code 401 for asset response.*"),
      EAutomationExpectedErrorFlags::Contains,
      occurrences);

  // Create a tileset, expect the default Georeference to be
  // created, and then delete them both
  {
    AActor* tileset = GEditor->AddActor(
        currentLevel,
        ACesium3DTileset::StaticClass(),
        IdentityTransform);
    TestNotNull(TEXT("The Cesium3DTileset instance could be created"), tileset);

    ACesiumGeoreference* georeference = FindObject<ACesiumGeoreference>(
        currentLevel,
        TEXT("CesiumGeoreferenceDefault"));
    TestNotNull(
        TEXT("The default CesiumGeoreference instance was created"),
        georeference);
    TestTrue(
        TEXT("The default CesiumGeoreference instance is valid"),
        IsValid(georeference));

    World->DestroyActor(georeference);
    TestFalse(
        TEXT(
            "After deletion, the default CesiumGeoreference instance is NOT valid"),
        IsValid(georeference));

    World->DestroyActor(tileset);
    TestFalse(
        TEXT("After deletion, the Cesium3DTileset instance is NOT valid"),
        IsValid(tileset));
  }

  // Create a new tileset, and expect a new default
  // Georeference to be created
  {
    AActor* secondTileset = GEditor->AddActor(
        currentLevel,
        ACesium3DTileset::StaticClass(),
        IdentityTransform);
    TestNotNull(
        TEXT("The second Cesium3DTileset instance could be created"),
        secondTileset);

    // NOTE (TODO?) : We're relying on the default suffix "_0" for the name
    // here!
    ACesiumGeoreference* secondGeoreference = FindObject<ACesiumGeoreference>(
        currentLevel,
        TEXT("CesiumGeoreferenceDefault_0"));
    TestNotNull(
        TEXT("The second default CesiumGeoreference instance was created"),
        secondGeoreference);
    TestTrue(
        TEXT("The second default CesiumGeoreference instance is valid"),
        IsValid(secondGeoreference));
  }

  // Clean up by creating a new map.
  // (TODO: Does this make sense?)
  FAutomationEditorCommonUtils::CreateNewMap();
  // GLevelEditorModeTools().MapChangeNotify();

  return true;
}

//============================================================================

/*
 * A test for https://github.com/CesiumGS/cesium-unreal/issues/498
 *
 * - Spawns two Georeferenced actors at different locations
 * - Checks that they indeed end up at their spawn location
 *
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCesiumGeoreferenceComponentSpawningTest,
    "Cesium.Georeference.SpawningGeorefActors",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter);

bool FCesiumGeoreferenceComponentSpawningTest::RunTest(
    const FString& Parameters) {

  UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
  ULevel* currentLevel = World->GetCurrentLevel();

  {
    const FVector expectedLocation0(100.0, 200.0f, 300.0f);
    AActor* actor0 = GEditor->AddActor(
        currentLevel,
        ACesiumGeoreferenceComponentTestActor::StaticClass(),
        FTransform(expectedLocation0));
    TestNotNull(
        TEXT(
            "The CesiumGeoreferenceComponentTestActor instance 0 could be created"),
        actor0);

    const FVector expectedLocation1(400.0, 500.0f, 600.0f);
    AActor* actor1 = GEditor->AddActor(
        currentLevel,
        ACesiumGeoreferenceComponentTestActor::StaticClass(),
        FTransform(expectedLocation1));
    TestNotNull(
        TEXT(
            "The CesiumGeoreferenceComponentTestActor instance 1 could be created"),
        actor1);

    const FVector actualLocation0 = actor0->GetTransform().GetLocation();
    TestEqual(
        TEXT(
            "The CesiumGeoreferenceComponentTestActor instance 0 is at the expected location"),
        actualLocation0,
        expectedLocation0);

    const FVector actualLocation1 = actor1->GetTransform().GetLocation();
    TestEqual(
        TEXT(
            "The CesiumGeoreferenceComponentTestActor instance 1 is at the expected location"),
        actualLocation1,
        expectedLocation1);
  }

  // Clean up by creating a new map.
  // (TODO: Does this make sense?)
  FAutomationEditorCommonUtils::CreateNewMap();

  return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
