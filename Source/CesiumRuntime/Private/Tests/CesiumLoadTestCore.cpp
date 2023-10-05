// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#if WITH_EDITOR

#include "CesiumLoadTestCore.h"

#include "CesiumAsync/ICacheDatabase.h"
#include "CesiumRuntime.h"

#include "Editor.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"

namespace Cesium {

struct LoadTestContext {
  SceneGenerationContext creationContext;
  SceneGenerationContext playContext;

  float cameraFieldOfView = 90.0f;

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
    context.playContext.syncWorldCamera();

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

    context.testInProgress = false;

    // Command is done
    return true;
  }

  // Let world tick, we'll come back to this command
  return false;
}

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(
    TestCleanupCommand,
    LoadTestContext&,
    context);
bool TestCleanupCommand::Update() {
  // Turn on the editor tileset updates so we can see what we loaded
  gLoadTestContext.creationContext.setSuspendUpdate(false);
  return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND(WaitForPIECommand);
bool WaitForPIECommand::Update() {
  if (!GEditor || !GEditor->IsPlayingSessionInEditor())
    return false;
  UE_LOG(LogCesium, Display, TEXT("Play in Editor ready..."));
  return true;
}

void clearCacheDb() {
  std::shared_ptr<CesiumAsync::ICacheDatabase> pCacheDatabase =
      getCacheDatabase();
  pCacheDatabase->clearAll();
}

bool RunLoadTest(
    const FString& testName,
    std::function<void(SceneGenerationContext&)> locationSetup,
    const std::vector<TestPass>& testPasses,
    int viewportWidth,
    int viewportHeight) {

  gLoadTestContext.reset();

  //
  // Programmatically set up the world
  //
  UE_LOG(LogCesium, Display, TEXT("Creating common world objects..."));
  createCommonWorldObjects(gLoadTestContext.creationContext);

  // Configure location specific objects
  UE_LOG(LogCesium, Display, TEXT("Setting up location..."));
  locationSetup(gLoadTestContext.creationContext);
  gLoadTestContext.creationContext.trackForPlay();

  // Halt tileset updates and reset them
  gLoadTestContext.creationContext.setSuspendUpdate(true);
  gLoadTestContext.creationContext.refreshTilesets();
  clearCacheDb();

  // Let the editor viewports see the same thing the test will
  gLoadTestContext.creationContext.syncWorldCamera();

  //
  // Start async commands
  //

  // Wait for shaders. Shader compiles could affect performance
  ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling);

  // Queue play in editor and set desired viewport size
  FRequestPlaySessionParams Params;
  Params.WorldType = EPlaySessionWorldType::PlayInEditor;
  Params.EditorPlaySettings = NewObject<ULevelEditorPlaySettings>();
  Params.EditorPlaySettings->NewWindowWidth = viewportWidth;
  Params.EditorPlaySettings->NewWindowHeight = viewportHeight;
  Params.EditorPlaySettings->EnableGameSound = false;
  GEditor->RequestPlaySession(Params);

  // Wait until PIE is ready
  ADD_LATENT_AUTOMATION_COMMAND(WaitForPIECommand());

  std::vector<TestPass>::const_iterator it;
  for (it = testPasses.begin(); it != testPasses.end(); ++it) {
    const TestPass& pass = *it;

    // Wait to show distinct gap in profiler
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f));

    // Do our timing capture
    FString loggingName = testName + ":" + pass.name;

    ADD_LATENT_AUTOMATION_COMMAND(TimeLoadingCommand(
        loggingName,
        gLoadTestContext,
        pass.setupStep,
        pass.verifyStep));
  }

  // Wait to show distinct gap in profiler
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.0f));

  // End play in editor
  ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());

  ADD_LATENT_AUTOMATION_COMMAND(TestCleanupCommand(gLoadTestContext));

  return true;
}

}; // namespace Cesium

#endif
