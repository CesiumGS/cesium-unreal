// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#if WITH_EDITOR

#include "CesiumLoadTestCore.h"

#include "Engine/World.h"
#include "Misc/AutomationTest.h"

#include "Cesium3DTileset.h"
#include "CesiumAsync/ICacheDatabase.h"
#include "CesiumRuntime.h"
#include "CesiumSunSky.h"

using namespace Cesium;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGoogleTilesPompidou,
    "Cesium.Performance.GoogleTiles.LocalePompidou",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGoogleTilesChrysler,
    "Cesium.Performance.GoogleTiles.LocaleChrysler",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGoogleTilesChryslerWarm,
    "Cesium.Performance.GoogleTiles.LocaleChrysler (Warm)",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGoogleTilesGuggenheim,
    "Cesium.Performance.GoogleTiles.LocaleGuggenheim",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGoogleTilesDeathValley,
    "Cesium.Performance.GoogleTiles.LocaleDeathValley",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGoogleTilesDeathValleyWarm,
    "Cesium.Performance.GoogleTiles.LocaleDeathValley (Warm)",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGoogleTilesTokyo,
    "Cesium.Performance.GoogleTiles.LocaleTokyo",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGoogleTilesGoogleplex,
    "Cesium.Performance.GoogleTiles.LocaleGoogleplex",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGoogleTilesMaxTileLoads,
    "Cesium.Performance.GoogleTiles.VaryMaxTileLoads",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

#define TEST_SCREEN_WIDTH 1280
#define TEST_SCREEN_HEIGHT 720

bool FGoogleTilesPompidou::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", GoogleTilesTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      GoogleTilesTestSetup::setupForPompidou,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FGoogleTilesChrysler::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", GoogleTilesTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      GoogleTilesTestSetup::setupForChrysler,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FGoogleTilesChryslerWarm::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(TestPass{
      "Warm Cache",
      GoogleTilesTestSetup::setupRefreshTilesets,
      nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      GoogleTilesTestSetup::setupForChrysler,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FGoogleTilesGuggenheim::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", GoogleTilesTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      GoogleTilesTestSetup::setupForGuggenheim,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FGoogleTilesDeathValley::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", GoogleTilesTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      GoogleTilesTestSetup::setupForDeathValley,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FGoogleTilesDeathValleyWarm::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(TestPass{
      "Warm Cache",
      GoogleTilesTestSetup::setupRefreshTilesets,
      nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      GoogleTilesTestSetup::setupForDeathValley,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FGoogleTilesTokyo::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", GoogleTilesTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      GoogleTilesTestSetup::setupForTokyo,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FGoogleTilesGoogleplex::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", GoogleTilesTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      GoogleTilesTestSetup::setupForGoogleplex,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FGoogleTilesMaxTileLoads::RunTest(const FString& Parameters) {
  auto setupPass = [this](
                       SceneGenerationContext& context,
                       TestPass::TestingParameter parameter) {
    std::shared_ptr<CesiumAsync::ICacheDatabase> pCacheDatabase =
        getCacheDatabase();
    pCacheDatabase->clearAll();

    int maxLoadsTarget = std::get<int>(parameter);
    context.setMaximumSimultaneousTileLoads(maxLoadsTarget);

    context.refreshTilesets();
  };

  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Default", GoogleTilesTestSetup::setupClearCache, NULL});
  testPasses.push_back(TestPass{"12", setupPass, NULL, 12});
  testPasses.push_back(TestPass{"16", setupPass, NULL, 16});
  testPasses.push_back(TestPass{"20", setupPass, NULL, 20});
  testPasses.push_back(TestPass{"24", setupPass, NULL, 24});
  testPasses.push_back(TestPass{"28", setupPass, NULL, 28});

  return RunLoadTest(
      GetBeautifiedTestName(),
      GoogleTilesTestSetup::setupForChrysler,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

#endif
