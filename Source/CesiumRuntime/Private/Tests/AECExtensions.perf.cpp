// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#if WITH_EDITOR

#include "CesiumLoadTestCore.h"

#include "Engine/World.h"
#include "Misc/AutomationTest.h"

#include "AECPerformanceSetup.h"
#include "Cesium3DTileset.h"
#include "CesiumAsync/ICacheDatabase.h"
#include "CesiumRuntime.h"
#include "CesiumSunSky.h"

using namespace Cesium;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetA0,
    "Cesium.Performance.Tileset Loading.AEC Model A0",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetA1,
    "Cesium.Performance.Tileset Loading.AEC Model A1",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetB0,
    "Cesium.Performance.Tileset Loading.AEC Model B0",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetB1,
    "Cesium.Performance.Tileset Loading.AEC Model B1",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetC0,
    "Cesium.Performance.Tileset Loading.AEC Model C0",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetC1,
    "Cesium.Performance.Tileset Loading.AEC Model C1",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetD0,
    "Cesium.Performance.Tileset Loading.AEC Model D0",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetD1,
    "Cesium.Performance.Tileset Loading.AEC Model D1",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetE0,
    "Cesium.Performance.Tileset Loading.AEC Model E0",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetE1,
    "Cesium.Performance.Tileset Loading.AEC Model E1",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetF0,
    "Cesium.Performance.Tileset Loading.AEC Model F0",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetF1,
    "Cesium.Performance.Tileset Loading.AEC Model F1",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetG0,
    "Cesium.Performance.Tileset Loading.AEC Model G0",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetG1,
    "Cesium.Performance.Tileset Loading.AEC Model G1",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetH0,
    "Cesium.Performance.Tileset Loading.AEC Model H0",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetH1,
    "Cesium.Performance.Tileset Loading.AEC Model H1",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetI0,
    "Cesium.Performance.Tileset Loading.AEC Model I0",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetI1,
    "Cesium.Performance.Tileset Loading.AEC Model I1",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetJ0,
    "Cesium.Performance.Tileset Loading.AEC Model J0",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetJ1,
    "Cesium.Performance.Tileset Loading.AEC Model J1",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetK0,
    "Cesium.Performance.Tileset Loading.AEC Model K0",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetK1,
    "Cesium.Performance.Tileset Loading.AEC Model K1",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetM0,
    "Cesium.Performance.Tileset Loading.AEC Model M0",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetM1,
    "Cesium.Performance.Tileset Loading.AEC Model M1",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetN0,
    "Cesium.Performance.Tileset Loading.AEC Model N0",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetN1,
    "Cesium.Performance.Tileset Loading.AEC Model N1",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetO0,
    "Cesium.Performance.Tileset Loading.AEC Model O0",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetO1,
    "Cesium.Performance.Tileset Loading.AEC Model O1",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetP0,
    "Cesium.Performance.Tileset Loading.AEC Model P0",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetP1,
    "Cesium.Performance.Tileset Loading.AEC Model P1",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetQ0,
    "Cesium.Performance.Tileset Loading.AEC Model Q0",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetQ1,
    "Cesium.Performance.Tileset Loading.AEC Model Q1",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetR0,
    "Cesium.Performance.Tileset Loading.AEC Model R0",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLoadTilesetR1,
    "Cesium.Performance.Tileset Loading.AEC Model R1",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

#define TEST_SCREEN_WIDTH 1280
#define TEST_SCREEN_HEIGHT 720

bool FLoadTilesetA0::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(TestPass{
      "Cold Cache",
      AECPerformanceTestSetup::setupClearCache,
      nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetA0,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FLoadTilesetA1::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", AECPerformanceTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetA1,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FLoadTilesetB0::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", AECPerformanceTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetB0,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FLoadTilesetB1::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", AECPerformanceTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetB1,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FLoadTilesetC0::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", AECPerformanceTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetC0,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FLoadTilesetC1::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", AECPerformanceTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetC1,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FLoadTilesetD0::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", AECPerformanceTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetD0,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FLoadTilesetD1::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", AECPerformanceTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetD1,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FLoadTilesetE0::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", AECPerformanceTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetE0,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FLoadTilesetE1::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", AECPerformanceTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetE1,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FLoadTilesetF0::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", AECPerformanceTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetF0,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FLoadTilesetF1::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", AECPerformanceTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetF1,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FLoadTilesetG0::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", AECPerformanceTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetG0,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FLoadTilesetG1::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", AECPerformanceTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetG1,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FLoadTilesetH0::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", AECPerformanceTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetH0,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FLoadTilesetH1::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", AECPerformanceTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetH1,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FLoadTilesetI0::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", AECPerformanceTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetI0,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FLoadTilesetI1::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", AECPerformanceTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetI1,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FLoadTilesetJ0::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", AECPerformanceTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetJ0,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FLoadTilesetJ1::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", AECPerformanceTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetJ1,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FLoadTilesetK0::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", AECPerformanceTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetK0,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FLoadTilesetK1::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", AECPerformanceTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetK1,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FLoadTilesetM0::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", AECPerformanceTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetM0,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FLoadTilesetM1::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", AECPerformanceTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetM1,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FLoadTilesetN0::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", AECPerformanceTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetN0,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FLoadTilesetN1::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", AECPerformanceTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetN1,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FLoadTilesetO0::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", AECPerformanceTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetO0,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FLoadTilesetO1::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", AECPerformanceTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetO1,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FLoadTilesetP0::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", AECPerformanceTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetP0,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FLoadTilesetP1::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", AECPerformanceTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetP1,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FLoadTilesetQ0::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", AECPerformanceTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetQ0,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FLoadTilesetQ1::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", AECPerformanceTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetQ1,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FLoadTilesetR0::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", AECPerformanceTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetR0,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

bool FLoadTilesetR1::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Cold Cache", AECPerformanceTestSetup::setupClearCache, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      AECPerformanceTestSetup::setupForTilesetR1,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

#endif
