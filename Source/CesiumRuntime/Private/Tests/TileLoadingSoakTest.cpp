// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#if WITH_EDITOR

#include "Math/UnrealMathUtility.h"
#include "Misc/AutomationTest.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"

#include "CesiumFlyToComponent.h"
#include "CesiumGeoreference.h"
#include "GlobeAwareDefaultPawn.h"

#include "CesiumLoadTestCore.h"
#include "CesiumSceneGeneration.h"
#include "CesiumTestHelpers.h"

#define VIEWPORT_WIDTH 1280;
#define VIEWPORT_HEIGHT 720;
// twelve hour stress test
constexpr static double STRESS_TEST_DURATION = 60 * 60 * 12;

namespace Cesium {
// Since this shares a name with the global defined in Google3dTilesLoadTest, we
// tell the compiler to look for the variable there instead.
extern LoadTestContext gLoadTestContext;

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(
    FPerformStressTestCommand,
    LoadTestContext&,
    context);

bool FPerformStressTestCommand::Update() {
  if (FPlatformTime::Seconds() > (this->StartTime + STRESS_TEST_DURATION)) {
    // Done with stress test, end command
    return true;
  }

  UCesiumFlyToComponent* flyTo =
      context.playContext.pawn->FindComponentByClass<UCesiumFlyToComponent>();

  if (flyTo->IsFlightInProgress()) {
    // We're still flying somewhere, give it some time to complete
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.0f));
    return false;
  }

  // Give it some time for the tiles to load where we are
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(10.0f));

  FVector pawnPosition = context.playContext.pawn->GetActorLocation();
  FVector llhPosition =
      context.playContext.georeference
          ->TransformUnrealPositionToLongitudeLatitudeHeight(pawnPosition);
  // If longitude is greater than 0, we're in the northern hemisphere
  bool northernHemisphere = llhPosition.Y >= 0;
  // Calculate a new random position to fly to in the opposite hemisphere
  FVector targetLlh(
      FMath::RandRange(-180.0f, 180.0f),
      FMath::RandRange(0.0f, 90.0f) * (northernHemisphere ? -1.0f : 1.0f),
      1000.0f);

  // Start the flight
  context.playContext.pawn
      ->FlyToLocationLongitudeLatitudeHeight(targetLlh, 0, 0, false);
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(flyTo->Duration));
  return false;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGoogleTilesStressTest,
    "Cesium.Performance.StressTest.GoogleTiles",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

bool FGoogleTilesStressTest::RunTest(const FString& Parameters) {

  LoadTestContext& context = gLoadTestContext;

  context.reset();

  UE_LOG(LogCesium, Display, TEXT("Creating common world objects..."));
  createCommonWorldObjects(context.creationContext);

  UE_LOG(LogCesium, Display, TEXT("Setting up location..."));
  GoogleTilesTestSetup::setupForGoogleplex(context.creationContext);
  context.creationContext.trackForPlay();

  // Halt tileset updates and reset them
  context.creationContext.setSuspendUpdate(true);
  context.creationContext.refreshTilesets();

  // Let the editor viewports see the same thing the test will
  context.creationContext.syncWorldCamera();

  ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling);

  // Queue play in editor and set desired viewport size
  FRequestPlaySessionParams Params;
  Params.WorldType = EPlaySessionWorldType::PlayInEditor;
  Params.EditorPlaySettings = NewObject<ULevelEditorPlaySettings>();
  Params.EditorPlaySettings->NewWindowWidth = VIEWPORT_WIDTH;
  Params.EditorPlaySettings->NewWindowHeight = VIEWPORT_HEIGHT;
  Params.EditorPlaySettings->EnableGameSound = false;
  GEditor->RequestPlaySession(Params);

  ADD_LATENT_AUTOMATION_COMMAND(
      InitForPlayWhenReady(context.creationContext, context.playContext));

  // Wait to show distinct gap in profiler
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.0f));

  ADD_LATENT_AUTOMATION_COMMAND(FPerformStressTestCommand(context));

  // End play in editor
  ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());

  ADD_LATENT_AUTOMATION_COMMAND(TestCleanupCommand(context));

  return true;
}

} // namespace Cesium

#endif
