// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#if WITH_EDITOR

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"

#include "CesiumAsync/ICacheDatabase.h"
#include "CesiumGltfComponent.h"
#include "CesiumRuntime.h"
#include "CesiumSceneGeneration.h"
#include "CesiumTestHelpers.h"
#include "GlobeAwareDefaultPawn.h"

using namespace Cesium;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCesiumLoadTestDenver,
    "Cesium.Performance.LoadTestDenver",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCesiumLoadTestGoogleplex,
    "Cesium.Performance.LoadTestGoogleplex",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCesiumLoadTestMontrealPointCloud,
    "Cesium.Performance.LoadTestMontrealPointCloud",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

struct LoadTestContext {
  SceneGenerationContext creationContext;
  SceneGenerationContext playContext;

  bool testInProgress;
  double startMark;
  double endMark;

  void reset() {
    creationContext = playContext = SceneGenerationContext();
    testInProgress = false;
    startMark = endMark = 0;
  }
};

LoadTestContext gLoadTestContext;

DEFINE_LATENT_AUTOMATION_COMMAND_FOUR_PARAMETER(
    TimeLoadingCommand,
    FString,
    loggingName,
    LoadTestContext&,
    context,
    std::function<void(SceneGenerationContext&)>,
    setupStep,
    std::function<void(SceneGenerationContext&)>,
    verifyStep);
bool TimeLoadingCommand::Update() {

  if (!context.testInProgress) {

    // Bind all play in editor pointers
    context.playContext.initForPlay(context.creationContext);

    if (setupStep)
      setupStep(context.playContext);

    // Start test mark, turn updates back on
    context.startMark = FPlatformTime::Seconds();
    UE_LOG(LogCesium, Display, TEXT("-- Load start mark -- %s"), *loggingName);

    context.playContext.setSuspendUpdate(false);

    context.testInProgress = true;

    // Return, let world tick
    return false;
  }

  double timeMark = FPlatformTime::Seconds();
  double testElapsedTime = timeMark - context.startMark;

  // The command is over if tilesets are loaded, or timed out
  // Wait for a maximum of 20 seconds
  const size_t testTimeout = 20;
  bool tilesetsloaded = context.playContext.areTilesetsDoneLoading();
  bool timedOut = testElapsedTime >= testTimeout;

  if (tilesetsloaded || timedOut) {
    context.endMark = timeMark;
    UE_LOG(LogCesium, Display, TEXT("-- Load end mark -- %s"), *loggingName);

    if (timedOut) {
      UE_LOG(
          LogCesium,
          Error,
          TEXT("TIMED OUT: Loading stopped after %.2f seconds"),
          testElapsedTime);
    } else {
      UE_LOG(
          LogCesium,
          Display,
          TEXT("Tileset load completed in %.2f seconds"),
          testElapsedTime);
    }

    if (verifyStep)
      verifyStep(context.playContext);

    // Turn on the editor tileset updates so we can see what we loaded
    gLoadTestContext.creationContext.setSuspendUpdate(false);

    context.testInProgress = false;

    // Command is done
    return true;
  }

  // Let world tick, we'll come back to this command
  return false;
}

struct TestPass {
  FString name;
  std::function<void(SceneGenerationContext&)> setupStep;
  std::function<void(SceneGenerationContext&)> verifyStep;
};

bool RunLoadTest(
    const FString& testName,
    std::function<void(SceneGenerationContext&)> locationSetup,
    const std::vector<TestPass>& testPasses) {

  gLoadTestContext.reset();

  //
  // Programmatically set up the world
  //
  UE_LOG(LogCesium, Display, TEXT("Creating world objects..."));
  createCommonWorldObjects(gLoadTestContext.creationContext);

  // Configure location specific objects
  locationSetup(gLoadTestContext.creationContext);
  gLoadTestContext.creationContext.trackForPlay();

  // Halt tileset updates and reset them
  gLoadTestContext.creationContext.setSuspendUpdate(true);
  gLoadTestContext.creationContext.refreshTilesets();

  //
  // Start async commands
  //

  // Start play in editor (don't sim in editor)
  ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));

  std::vector<TestPass>::const_iterator it;
  for (it = testPasses.begin(); it != testPasses.end(); ++it) {
    const TestPass& pass = *it;

    // Wait a bit
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.0f));

    // Do our timing capture
    FString loggingName = testName + ":" + pass.name;

    ADD_LATENT_AUTOMATION_COMMAND(TimeLoadingCommand(
        loggingName,
        gLoadTestContext,
        pass.setupStep,
        pass.verifyStep));
  }

  // End play in editor
  ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());

  return true;
}

void clearCacheDb(SceneGenerationContext& context) {
  std::shared_ptr<CesiumAsync::ICacheDatabase> pCacheDatabase =
      getCacheDatabase();
  pCacheDatabase->clearAll();
}

void refreshTilesets(SceneGenerationContext& context) {
  gLoadTestContext.playContext.refreshTilesets();
}

bool FCesiumLoadTestDenver::RunTest(const FString& Parameters) {

  std::vector<TestPass> testPasses;
  testPasses.push_back(TestPass{"Cold Cache", clearCacheDb, nullptr});
  testPasses.push_back(TestPass{"Warm Cache", refreshTilesets, nullptr});

  return RunLoadTest(GetTestName(), setupForDenver, testPasses);
}

bool FCesiumLoadTestGoogleplex::RunTest(const FString& Parameters) {

  std::vector<TestPass> testPasses;
  testPasses.push_back(TestPass{"Cold Cache", clearCacheDb, nullptr});
  testPasses.push_back(TestPass{"Warm Cache", refreshTilesets, nullptr});

  return RunLoadTest(GetTestName(), setupForGoogleTiles, testPasses);
}

bool FCesiumLoadTestMontrealPointCloud::RunTest(const FString& Parameters) {

  auto adjustCamera = [this](SceneGenerationContext& context) {
    // Zoom way out
    FCesiumCamera zoomedOut;
    zoomedOut.ViewportSize = FVector2D(1024, 768);
    zoomedOut.Location = FVector(0, 0, 7240000.0);
    zoomedOut.Rotation = FRotator(-90.0, 0.0, 0.0);
    zoomedOut.FieldOfViewDegrees = 90;
    context.setCamera(zoomedOut);

    context.pawn->SetActorLocation(zoomedOut.Location);
  };

  auto verifyVisibleTiles = [this](SceneGenerationContext& context) {
    Cesium3DTilesSelection::Tileset* pTileset =
        context.tilesets[0]->GetTileset();
    if (TestNotNull("Tileset", pTileset)) {
      int visibleTiles = 0;
      pTileset->forEachLoadedTile([&](Cesium3DTilesSelection::Tile& tile) {
        if (tile.getState() != Cesium3DTilesSelection::TileLoadState::Done)
          return;
        const Cesium3DTilesSelection::TileContent& content = tile.getContent();
        const Cesium3DTilesSelection::TileRenderContent* pRenderContent =
            content.getRenderContent();
        if (!pRenderContent) {
          return;
        }

        UCesiumGltfComponent* Gltf = static_cast<UCesiumGltfComponent*>(
            pRenderContent->getRenderResources());
        if (Gltf && Gltf->IsVisible()) {
          ++visibleTiles;
        }
      });

      TestEqual("visibleTiles", visibleTiles, 1);
    }
  };

  std::vector<TestPass> testPasses;
  testPasses.push_back(TestPass{"Cold Cache", clearCacheDb, nullptr});
  testPasses.push_back(TestPass{"Adjust", adjustCamera, verifyVisibleTiles});

  return RunLoadTest(GetTestName(), setupForMontrealPointCloud, testPasses);
}

#endif
