// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#if WITH_EDITOR

#include "CesiumLoadTestCore.h"

#include "Engine/World.h"
#include "Misc/AutomationTest.h"

#include "Cesium3DTileset.h"

using namespace Cesium;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGoogleTilesPompidou,
    "Cesium.Performance.GoogleTilesPompidou",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGoogleTilesChrysler,
    "Cesium.Performance.GoogleTilesChrysler",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGoogleTilesGuggenheim,
    "Cesium.Performance.GoogleTilesGuggenheim",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGoogleTilesDeathValley,
    "Cesium.Performance.GoogleTilesDeathValley",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGoogleTilesTokyo,
    "Cesium.Performance.GoogleTilesTokyo",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

void setupForPompidou(SceneGenerationContext& context) {
  context.setCommonProperties(
      FVector(2.352200, 48.860600, 200),
      FVector(0, 0, 0),
      FRotator(-20.0, 0.0, 0.0),
      60.0f);

  ACesium3DTileset* tileset = context.world->SpawnActor<ACesium3DTileset>();
  tileset->SetUrl(Cesium::SceneGenerationContext::testGoogleUrl);
  tileset->SetTilesetSource(ETilesetSource::FromUrl);
  tileset->SetActorLabel(TEXT("Center Pompidou, Paris, France"));
  context.tilesets.push_back(tileset);
}

void setupForChrysler(SceneGenerationContext& context) {
  context.setCommonProperties(
      FVector(-73.9752624659, 40.74697185903, 307.38),
      FVector(0, 0, 0),
      FRotator(-15.0, 0.0, 0.0),
      60.0f);

  ACesium3DTileset* tileset = context.world->SpawnActor<ACesium3DTileset>();
  tileset->SetUrl(Cesium::SceneGenerationContext::testGoogleUrl);
  tileset->SetTilesetSource(ETilesetSource::FromUrl);
  tileset->SetActorLabel(TEXT("Chrysler Building, NYC"));
  context.tilesets.push_back(tileset);
}

void setupForGuggenheim(SceneGenerationContext& context) {
  context.setCommonProperties(
      FVector(-2.937, 43.2685, 150),
      FVector(0, 0, 0),
      FRotator(-15.0, 90.0, 0.0),
      60.0f);

  ACesium3DTileset* tileset = context.world->SpawnActor<ACesium3DTileset>();
  tileset->SetUrl(Cesium::SceneGenerationContext::testGoogleUrl);
  tileset->SetTilesetSource(ETilesetSource::FromUrl);
  tileset->SetActorLabel(TEXT("Guggenheim Museum, Bilbao, Spain"));
  context.tilesets.push_back(tileset);
}

void setupForDeathValley(SceneGenerationContext& context) {
  context.setCommonProperties(
      FVector(-116.812278, 36.42, 300),
      FVector(0, 0, 0),
      FRotator(0, 90.0, 0.0),
      60.0f);

  ACesium3DTileset* tileset = context.world->SpawnActor<ACesium3DTileset>();
  tileset->SetUrl(Cesium::SceneGenerationContext::testGoogleUrl);
  tileset->SetTilesetSource(ETilesetSource::FromUrl);
  tileset->SetActorLabel(
      TEXT("Zabriskie Point, Death Valley National Park, California"));
  context.tilesets.push_back(tileset);
}

void setupForTokyo(SceneGenerationContext& context) {
  context.setCommonProperties(
      FVector(139.7563178458, 35.652798383944, 525.62),
      FVector(0, 0, 0),
      FRotator(-15, 300, 0.0),
      60.0f);

  ACesium3DTileset* tileset = context.world->SpawnActor<ACesium3DTileset>();
  tileset->SetUrl(Cesium::SceneGenerationContext::testGoogleUrl);
  tileset->SetTilesetSource(ETilesetSource::FromUrl);
  tileset->SetActorLabel(TEXT("Tokyo Tower, Tokyo, Japan"));
  context.tilesets.push_back(tileset);
}

void refreshTilesets(SceneGenerationContext& context) {
  context.refreshTilesets();
}

bool FGoogleTilesPompidou::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(TestPass{"Cold Cache", nullptr, nullptr});
  testPasses.push_back(TestPass{"Warm Cache", refreshTilesets, nullptr});

  return RunLoadTest(GetTestName(), setupForPompidou, testPasses, 1280, 720);
}

bool FGoogleTilesChrysler::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(TestPass{"Cold Cache", nullptr, nullptr});
  testPasses.push_back(TestPass{"Warm Cache", refreshTilesets, nullptr});

  return RunLoadTest(GetTestName(), setupForChrysler, testPasses, 1280, 720);
}

bool FGoogleTilesGuggenheim::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(TestPass{"Cold Cache", nullptr, nullptr});
  testPasses.push_back(TestPass{"Warm Cache", refreshTilesets, nullptr});

  return RunLoadTest(GetTestName(), setupForGuggenheim, testPasses, 1280, 720);
}

bool FGoogleTilesDeathValley::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(TestPass{"Cold Cache", nullptr, nullptr});
  testPasses.push_back(TestPass{"Warm Cache", refreshTilesets, nullptr});

  return RunLoadTest(GetTestName(), setupForDeathValley, testPasses, 1280, 720);
}

bool FGoogleTilesTokyo::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(TestPass{"Cold Cache", nullptr, nullptr});
  testPasses.push_back(TestPass{"Warm Cache", refreshTilesets, nullptr});

  return RunLoadTest(GetTestName(), setupForTokyo, testPasses, 1280, 720);
}

#endif
