// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#if WITH_EDITOR

#include "CesiumLoadTestCore.h"

#include "Engine/World.h"
#include "Misc/AutomationTest.h"

#include "Cesium3DTileset.h"
#include "CesiumAsync/ICacheDatabase.h"
#include "CesiumRuntime.h"
#include "CesiumSunSky.h"
#include "GoogleTilesTestSetup.h"

using namespace Cesium;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetGooglePompidou,
    "Cesium.Performance.Tileset Loading.Google P3DT Pompidou",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetGoogleChrysler,
    "Cesium.Performance.Tileset Loading.Google P3DT Chrysler",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetGoogleChryslerWarm,
    "Cesium.Performance.Tileset Loading.Google P3DT Chrysler, warm cache",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetGoogleGuggenheim,
    "Cesium.Performance.Tileset Loading.Google P3DT Guggenheim",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetGoogleDeathValley,
    "Cesium.Performance.Tileset Loading.Google P3DT DeathValley",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetGoogleDeathValleyWarm,
    "Cesium.Performance.Tileset Loading.Google P3DT DeathValley, warm cache",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetGoogleTokyo,
    "Cesium.Performance.Tileset Loading.Google P3DT Tokyo",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetGoogleGoogleplex,
    "Cesium.Performance.Tileset Loading.Google P3DT Googleplex",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetGoogleChryslerVaryMaxTileLoads,
    "Cesium.Performance.Tileset Loading.Google P3DT Chrysler, vary max tile loads",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

#define TEST_SCREEN_WIDTH 1280
#define TEST_SCREEN_HEIGHT 720

bool FLoadTilesetGooglePompidou::RunTest(const FString& Parameters) {
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

bool FLoadTilesetGoogleChrysler::RunTest(const FString& Parameters) {
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

bool FLoadTilesetGoogleChryslerWarm::RunTest(const FString& Parameters) {
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

bool FLoadTilesetGoogleGuggenheim::RunTest(const FString& Parameters) {
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

bool FLoadTilesetGoogleDeathValley::RunTest(const FString& Parameters) {
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

bool FLoadTilesetGoogleDeathValleyWarm::RunTest(const FString& Parameters) {
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

bool FLoadTilesetGoogleTokyo::RunTest(const FString& Parameters) {
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

bool FLoadTilesetGoogleGoogleplex::RunTest(const FString& Parameters) {
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

bool FLoadTilesetGoogleChryslerVaryMaxTileLoads::RunTest(
    const FString& Parameters) {
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
