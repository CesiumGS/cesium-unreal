// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#if WITH_EDITOR

#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"

#include "CesiumTestHelpers.h"
#include "CesiumSceneGeneration.h"
#include "CesiumGltfComponent.h"
#include "CesiumRuntime.h"
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

bool neverBreak(SceneGenerationContext& context) { return false; }

bool breakWhenTilesetsLoaded(SceneGenerationContext& context) {
  std::vector<ACesium3DTileset*>::const_iterator it;
  for (it = context.tilesets.begin(); it != context.tilesets.end(); ++it) {
    ACesium3DTileset* tileset = *it;

    int progress = (int)tileset->GetLoadProgress();
    if (progress != 100)
      return false;
  }
  return true;
}

bool tickWorldUntil(
    SceneGenerationContext& context,
    size_t timeoutSecs,
    std::function<bool(SceneGenerationContext&)> breakFunction) {
  const double minStepTime = 0.050; // Don't loop faster than 20 fps

  const double testStartMark = FPlatformTime::Seconds();
  const double testEndMark = testStartMark + (double)timeoutSecs;
  double lastTimeMark = testStartMark;

  while (1) {
    double frameTimeMark = FPlatformTime::Seconds();

    if (frameTimeMark > testEndMark)
      return true;

    double frameElapsedTime = frameTimeMark - lastTimeMark;

    if (frameElapsedTime < minStepTime) {
      double sleepTarget = minStepTime - frameElapsedTime;
      FPlatformProcess::Sleep(sleepTarget);
      continue;
    }

    //
    // Force a tick. Derived from various examples in this code base
    // Search for FTSTicker::GetCoreTicker().Tick
    //

    // Increment global frame counter once for each app tick.
    GFrameCounter++;

    // Let world tick at same rate as this loop
    context.world->Tick(ELevelTick::LEVELTICK_ViewportsOnly, frameElapsedTime);

    // Application tick
    FTaskGraphInterface::Get().ProcessThreadUntilIdle(
        ENamedThreads::GameThread);
    FTSTicker::GetCoreTicker().Tick(frameElapsedTime);

    // Let UI update
    // Not absolutely necessary, but convenient when running the tests
    // from inside the editor, so the UI doesn't appear frozen
    FSlateApplication::Get().PumpMessages();
    FSlateApplication::Get().Tick();

    if (breakFunction(context))
      return false;

    lastTimeMark = frameTimeMark;
  };
}

bool RunLoadTest(
    std::function<void(SceneGenerationContext&)> locationSetup,
    std::function<void(SceneGenerationContext&)> afterTest = {}) {

  //
  // Programmatically set up the world
  //
  UE_LOG(LogCesium, Display, TEXT("Creating world objects..."));
  Cesium::SceneGenerationContext context;
  Cesium::createCommonWorldObjects(context);

  // Configure location specific objects
  locationSetup(context);

  // Halt tileset updates and reset them
  context.setSuspendUpdate(true);
  context.refreshTilesets();

  // Immediately start a requested play session
  UE_LOG(LogCesium, Display, TEXT("Requesting play session..."));
  GEditor->RequestPlaySession(context.playSessionParams);
  GEditor->StartQueuedPlaySessionRequest();

  // Let world settle for 1 second
  UE_LOG(LogCesium, Display, TEXT("Letting world settle for 1 second..."));
  tickWorldUntil(context, 1, neverBreak);

  // Start test mark, turn updates back on
  double loadStartMark = FPlatformTime::Seconds();
  UE_LOG(LogCesium, Display, TEXT("-- Load start mark --"));
  context.setSuspendUpdate(false);

  // Spin for a maximum of 20 seconds, or until tilesets finish loading
  const size_t testTimeout = 20;
  UE_LOG(
      LogCesium,
      Display,
      TEXT("Tick world until tilesets load, or %d seconds elapse..."),
      testTimeout);
  bool timedOut = tickWorldUntil(context, testTimeout, breakWhenTilesetsLoaded);

  double loadEndMark = FPlatformTime::Seconds();
  UE_LOG(LogCesium, Display, TEXT("-- Load end mark --"));

  //
  // Skip object cleanup. Let all objects be available for viewing after test
  //
  // Let world settle for 1 second
  UE_LOG(LogCesium, Display, TEXT("Letting world settle for 1 second..."));
  tickWorldUntil(context, 1, neverBreak);

  UE_LOG(LogCesium, Display, TEXT("Ending play session..."));
  GEditor->RequestEndPlayMap();

  double loadElapsedTime = loadEndMark - loadStartMark;

  if (timedOut) {
    UE_LOG(
        LogCesium,
        Error,
        TEXT("TIMED OUT: Loading stopped after %.2f seconds"),
        loadElapsedTime);
  } else {
    UE_LOG(
        LogCesium,
        Display,
        TEXT("Tileset load completed in %.2f seconds"),
        loadElapsedTime);
  }

  if (afterTest) {
    afterTest(context);
  }

  return !timedOut;
}

bool FCesiumLoadTestDenver::RunTest(const FString& Parameters) {
  return RunLoadTest(setupForDenver);
}

bool FCesiumLoadTestGoogleplex::RunTest(const FString& Parameters) {
  return RunLoadTest(setupForGoogleTiles);
}

bool FCesiumLoadTestMontrealPointCloud::RunTest(const FString& Parameters) {

  auto after = [this](SceneGenerationContext& context) {
    // Zoom way out
    FCesiumCamera zoomedOut;
    zoomedOut.ViewportSize = FVector2D(1024, 768);
    zoomedOut.Location = FVector(0, 0, 7240000.0);
    zoomedOut.Rotation = FRotator(-90.0, 0.0, 0.0);
    zoomedOut.FieldOfViewDegrees = 90;
    context.setCamera(zoomedOut);

    context.pawn->SetActorLocation(zoomedOut.Location);

    context.setSuspendUpdate(false);
    tickWorldUntil(context, 10, breakWhenTilesetsLoaded);
    context.setSuspendUpdate(true);

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

  return RunLoadTest(setupForMelbourne, after);
}

#endif
