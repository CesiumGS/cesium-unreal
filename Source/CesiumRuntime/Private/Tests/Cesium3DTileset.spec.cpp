// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#if WITH_EDITOR

#include "Cesium3DTileset.h"
#include "CesiumGlobeAnchorComponent.h"
#include "CesiumGltfComponent.h"
#include "CesiumLoadTestCore.h"
#include "CesiumSceneGeneration.h"
#include "CesiumSunSky.h"
#include "CesiumTestHelpers.h"
#include "Engine/World.h"
#include "GlobeAwareDefaultPawn.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationTestSettings.h"
#include <Cesium3DTilesSelection/TilesetSharedAssetSystem.h>
#include <CesiumAsync/ICacheDatabase.h>

#define TEST_SCREEN_WIDTH 1280
#define TEST_SCREEN_HEIGHT 720

namespace Cesium {

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCesium3DTilesetSharedImages,
    "Cesium.Unit.3DTileset.SharedImages",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

static void setupForSharedImages(SceneGenerationContext& context) {
  context.setCommonProperties(
      FVector(21.16677692, -67.38013505, -6375355.1944),
      FVector(-12, -1300, -5),
      FRotator(0, 90, 0),
      60.0f);

  context.georeference->SetOriginEarthCenteredEarthFixed(FVector(0, 0, 0));
  context.pawn->SetActorLocation(FVector(485.0, 2400.0, 520.0));
  context.pawn->SetActorRotation(FQuat::MakeFromEuler(FVector(0, 0, 270)));

  context.sunSky->TimeZone = 9.0f;
  context.sunSky->UpdateSun();

  ACesiumGeoreference* georeference =
      context.world->SpawnActor<ACesiumGeoreference>();
  check(georeference != nullptr);
  georeference->SetOriginPlacement(EOriginPlacement::TrueOrigin);

  ACesium3DTileset* tileset = context.world->SpawnActor<ACesium3DTileset>();
  tileset->SetTilesetSource(ETilesetSource::FromCesiumIon);
  tileset->SetIonAssetID(2757071);
  tileset->SetIonAccessToken(SceneGenerationContext::testIonToken);

  tileset->SetActorLabel(TEXT("SharedImages"));
  tileset->SetGeoreference(georeference);
  tileset->SuspendUpdate = false;
  tileset->LogSelectionStats = true;
  context.tilesets.push_back(tileset);

  UCesiumGlobeAnchorComponent* GlobeAnchorComponent =
      NewObject<UCesiumGlobeAnchorComponent>(tileset, TEXT("GlobeAnchor"));
  tileset->AddInstanceComponent(GlobeAnchorComponent);
  GlobeAnchorComponent->SetAdjustOrientationForGlobeWhenMoving(false);
  GlobeAnchorComponent->SetGeoreference(georeference);
  GlobeAnchorComponent->RegisterComponent();
  GlobeAnchorComponent->MoveToEarthCenteredEarthFixedPosition(
      FVector(0.0, 0.0, 0.0));

  ADirectionalLight* Light = context.world->SpawnActor<ADirectionalLight>();
  Light->SetActorRotation(FQuat::MakeFromEuler(FVector(0, 0, 270)));
}

void tilesetPass(
    SceneGenerationContext& context,
    TestPass::TestingParameter parameter) {}

bool FCesium3DTilesetSharedImages::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(TestPass{"Refresh Pass", tilesetPass, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      setupForSharedImages,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCesium3DTilesetSnowdonBenchmark,
    "Cesium.Performance.3DTileset.SnowdonBenchmark",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter);

static void setupForSnowdon(SceneGenerationContext& context) {
  context.setCommonProperties(
      FVector(-79.8867314431, 40.0223377722, 197.1008007424),
      FVector(-293.823058, 6736.144397, 2730.501500),
      FRotator(-13.400000, -87.799997, 0.000000),
      60.0f);

  context.sunSky->TimeZone = 5.0f;
  context.sunSky->UpdateSun();

  ACesium3DTileset* tileset = context.world->SpawnActor<ACesium3DTileset>();
  tileset->SetTilesetSource(ETilesetSource::FromCesiumIon);
  tileset->SetIonAssetID(2758251);
  tileset->SetIonAccessToken(SceneGenerationContext::testIonToken);

  tileset->SetActorLabel(TEXT("Snowdon"));
  tileset->SuspendUpdate = false;
  tileset->LogSelectionStats = true;
  context.tilesets.push_back(tileset);

  ADirectionalLight* Light = context.world->SpawnActor<ADirectionalLight>();
  Light->SetActorRotation(FQuat::MakeFromEuler(FVector(0, 0, 270)));
}

void snowdonPass(
    SceneGenerationContext& context,
    TestPass::TestingParameter parameter) {}

bool FCesium3DTilesetSnowdonBenchmark::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(TestPass{"Refresh Pass", snowdonPass, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      setupForSnowdon,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

} // namespace Cesium

#endif
