// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#if WITH_EDITOR

#include <algorithm>
#include <random>
#include <span>
#include <stack>
#include <strstream>
#include <vector>

#include "Containers/UnrealString.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "GenericPlatform/GenericPlatformMemory.h"
#include "GenericPlatform/GenericPlatformProcess.h"
#include "HAL/PlatformFileManager.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/AutomationTest.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"

#include "Cesium3DTilesSelection/Tile.h"
#include "Cesium3DTileset.h"
#include "CesiumFlyToComponent.h"
#include "CesiumGeoreference.h"
#include "GlobeAwareDefaultPawn.h"
#include "GoogleTilesTestSetup.h"

#include "CesiumLoadTestCore.h"
#include "CesiumSceneGeneration.h"
#include "CesiumTestHelpers.h"

#include "TestRegionPolygons.h"

#define VIEWPORT_WIDTH 1280;
#define VIEWPORT_HEIGHT 720;
// Twelve hour soak test
constexpr static double SOAK_TEST_DURATION = 60 * 60 * 12;
// The duration in seconds between each stress test iteration
constexpr static double TEST_ITERATION_DELAY = 10.0;
constexpr static float FLIGHT_TIME = 5.0f;

// Stack of indices into TestRegionPolygons::polygons to use next
static std::stack<int> nextPolygonIndex;

namespace Cesium {

FString& getLogFilePath() {
  static std::optional<FString> path{};
  static const TCHAR* filename = TEXT("tiles_output.csv");

  if (!path.has_value()) {
    path = FString::Printf(TEXT("%s/%s"), *FPaths::ProjectDir(), filename);
  }

  return path.value();
}

void countTree(
    Cesium3DTilesSelection::Tile* tile,
    int depth,
    int& outCount,
    int& outUnloaded,
    int& outUnloading,
    int& outContentLoading,
    int& outContentLoaded,
    int& outDone) {
  /*std::string indentation(depth, '\t');
  UE_LOG(
      LogCesium,
         Display,
         TEXT("%s- tile load state %d"),
         UTF8_TO_TCHAR(indentation.c_str()),
         tile->getState());*/

  outCount++;
  std::span<Cesium3DTilesSelection::Tile> tiles = tile->getChildren();
  for (Cesium3DTilesSelection::Tile& child : tiles) {
    switch (child.getState()) {
    case Cesium3DTilesSelection::TileLoadState::Unloaded:
      outUnloaded++;
      break;
    case Cesium3DTilesSelection::TileLoadState::ContentLoading:
      outContentLoading++;
      break;
    case Cesium3DTilesSelection::TileLoadState::ContentLoaded:
      outContentLoaded++;
      break;
    case Cesium3DTilesSelection::TileLoadState::Unloading:
      outUnloading++;
      break;
    case Cesium3DTilesSelection::TileLoadState::Done:
      outDone++;
      break;
    }
    countTree(
        &child,
        depth + 1,
        outCount,
        outUnloaded,
        outUnloading,
        outContentLoading,
        outContentLoaded,
        outDone);
  }
}

void logDebug(ACesium3DTileset* tilesetActor) {
  Cesium3DTilesSelection::Tileset* tileset = tilesetActor->GetTileset();
  int numTiles = 0, numUnloaded = 0, numUnloading = 0, numContentLoading = 0,
      numContentLoaded = 0, numDone = 0;
  countTree(
      tileset->getRootTile(),
      0,
      numTiles,
      numUnloaded,
      numUnloading,
      numContentLoading,
      numContentLoaded,
      numDone);

  FGenericPlatformMemoryStats stats = FPlatformMemory::GetStats();

  IPlatformFile& file = FPlatformFileManager::Get().GetPlatformFile();
  IFileHandle* handle = file.OpenWrite(*getLogFilePath(), true);
  assert(handle);

  std::stringstream outstream;

  outstream << static_cast<uint64>(FPlatformTime::Seconds()) << "," << numTiles
            << "," << numUnloaded << "," << numUnloading << ","
            << numContentLoading << "," << numContentLoaded << "," << numDone
            << "," << stats.UsedVirtual << std::endl;

  std::string str = outstream.str();
  handle->Write(reinterpret_cast<const uint8*>(str.c_str()), str.length());
  handle->Flush();
  delete handle;

  UE_LOG(
      LogCesium,
      Display,
      TEXT(
          "Tileset has %d tiles in tree (%d unloaded, %d unloading, %d content loading, %d content loaded, %d done)"),
      numTiles,
      numUnloaded,
      numUnloading,
      numContentLoading,
      numContentLoaded,
      numDone);
}

void fillWithRandomIndices() {
  // Create a vector with every index
  std::vector<int> indices;
  for (int i = 0; i < TEST_REGION_POLYGONS_COUNT; i++) {
    indices.push_back(i);
  }

  // Shuffle indices
  std::default_random_engine rng{};
  std::shuffle(indices.begin(), indices.end(), rng);

  // Push shuffled indices onto stack
  for (int idx : indices) {
    nextPolygonIndex.push(idx);
  }
}

// Since this shares a name with the global defined in Google3dTilesLoadTest, we
// tell the compiler to look for the variable there instead.
extern LoadTestContext gLoadTestContext;

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(
    FFlyToRandomLocationCommand,
    LoadTestContext&,
    context);

bool FFlyToRandomLocationCommand::Update() {

  if (!GEditor->IsPlaySessionInProgress()) {
    return true;
  }

  logDebug(context.playContext.tilesets.at(0));

  UCesiumFlyToComponent* flyTo =
      context.playContext.pawn->FindComponentByClass<UCesiumFlyToComponent>();

  flyTo->Duration = FLIGHT_TIME;

  FVector pawnPosition = context.playContext.pawn->GetActorLocation();
  FVector llhPosition =
      context.playContext.georeference
          ->TransformUnrealPositionToLongitudeLatitudeHeight(pawnPosition);

  if (nextPolygonIndex.empty()) {
    fillWithRandomIndices();
  }

  const int nextIndex = nextPolygonIndex.top();
  nextPolygonIndex.pop();

  FVector targetLlh = TestRegionPolygons::polygons[nextIndex].GetRandomPoint();
  targetLlh.Z = 1000.0f;

  // Start the flight
  context.playContext.pawn
      ->FlyToLocationLongitudeLatitudeHeight(targetLlh, 0, 0, false);
  return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGoogleTilesStressTest,
    "Cesium.Performance.StressTest.GoogleTiles",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::StressFilter)

bool FGoogleTilesStressTest::RunTest(const FString& Parameters) {

  LoadTestContext& context = gLoadTestContext;

  context.reset();

  UE_LOG(LogCesium, Display, TEXT("Creating common world objects..."));
  createCommonWorldObjects(context.creationContext);

  UE_LOG(LogCesium, Display, TEXT("Setting up location..."));
  GoogleTilesTestSetup::setupForGoogleplex(context.creationContext);
  ACesium3DTileset* tileset = context.creationContext.tilesets.at(0);
  tileset->MaximumCachedBytes = 0;
  context.creationContext.trackForPlay();

  // Let the editor viewports see the same thing the test will
  context.creationContext.syncWorldCamera();

  context.creationContext.refreshTilesets();

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

  int numFlights = static_cast<int>(
      SOAK_TEST_DURATION / (FLIGHT_TIME + TEST_ITERATION_DELAY));

  for (int i = 0; i < numFlights; i++) {
    // Give it some time for the tiles to load where we are
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(TEST_ITERATION_DELAY));
    ADD_LATENT_AUTOMATION_COMMAND(FFlyToRandomLocationCommand(context));
    ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(FLIGHT_TIME));
  }

  // End play in editor
  ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());

  ADD_LATENT_AUTOMATION_COMMAND(TestCleanupCommand(context));

  return true;
}

} // namespace Cesium

#endif
