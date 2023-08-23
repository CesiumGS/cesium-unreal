// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#if WITH_EDITOR

#include "EngineAnalytics.h"
#include "Interfaces/IAnalyticsProvider.h"
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"

#include "CesiumRuntime.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCesiumLoadTest,
    "Cesium.LoadTest",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

bool FCesiumLoadTest::RunTest(const FString& Parameters) {
  FString MapName = "MyMap";
  double MapLoadStartTime = 0;

  // Test event for analytics. This should fire anytime this automation
  // procedure is started.

  if (FEngineAnalytics::IsAvailable()) {
    FEngineAnalytics::GetProvider().RecordEvent(TEXT("Editor.Usage.TestEvent"));
    UE_LOG(
        LogCesium,
        Log,
        TEXT(
            "AnayticsTest: Load All Maps automation triggered and Editor.Usage.TestEvent analytic event has been fired."));
  }

  {
    // Find the main editor window
    TArray<TSharedRef<SWindow>> AllWindows;
    FSlateApplication::Get().GetAllVisibleWindowsOrdered(AllWindows);
    if (AllWindows.Num() == 0) {
      UE_LOG(
          LogCesium,
          Error,
          TEXT("ERROR: Could not find the main editor window."));
      return false;
    }
    WindowScreenshotParameters WindowParameters;
    WindowParameters.CurrentWindow = AllWindows[0];

    // Disable Eye Adaptation
    static IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(
        TEXT("r.EyeAdaptationQuality"));
    CVar->Set(0);

    // Create a screen shot filename and path
    const FString LoadAllMapsTestName = FString::Printf(
        TEXT("LoadAllMaps_Editor/%s"),
        *FPaths::GetBaseFilename(MapName));
    WindowParameters.ScreenshotName =
        AutomationCommon::GetScreenshotName(LoadAllMapsTestName);

    // Get the current number of seconds.  This will be used to track how long
    // it took to load the map.
    MapLoadStartTime = FPlatformTime::Seconds();
    // Load the map
    FAutomationEditorCommonUtils::LoadMap(MapName);
    // Log how long it took to launch the map.
    UE_LOG(
        LogEditorAutomationTests,
        Display,
        TEXT("Map '%s' took %.3f to load"),
        *MapName,
        FPlatformTime::Seconds() - MapLoadStartTime);

    // If we don't have NoTextureStreaming enabled, give the textures some time
    // to load.
    if (!FParse::Param(FCommandLine::Get(), TEXT("NoTextureStreaming"))) {
      // Give the contents some time to load
      ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.5f));
    }
  }

  return true;
}

#endif
