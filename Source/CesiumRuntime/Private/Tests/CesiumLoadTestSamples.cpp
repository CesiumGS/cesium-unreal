// Copyright 2020-2023 CesiumGS, Inc. and Contributors

#if WITH_EDITOR

#include "CesiumLoadTestCore.h"

#include "Misc/AutomationTest.h"

#include "CesiumGltfComponent.h"
#include "GlobeAwareDefaultPawn.h"

using namespace Cesium;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCesiumSampleLoadDenver,
    "Cesium.Performance.SampleLoadDenver",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCesiumSampleLoadGoogleplex,
    "Cesium.Performance.SampleLoadGoogleplex",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCesiumSampleLoadMontrealPointCloud,
    "Cesium.Performance.SampleLoadMontrealPointCloud",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

void refreshSampleTilesets(SceneGenerationContext& context) {
  context.refreshTilesets();
}

bool FCesiumSampleLoadDenver::RunTest(const FString& Parameters) {

  std::vector<TestPass> testPasses;
  testPasses.push_back(TestPass{"Cold Cache", nullptr, nullptr});
  testPasses.push_back(TestPass{"Warm Cache", refreshSampleTilesets, nullptr});

  return RunLoadTest(GetTestName(), setupForDenver, testPasses);
}

bool FCesiumSampleLoadGoogleplex::RunTest(const FString& Parameters) {

  std::vector<TestPass> testPasses;
  testPasses.push_back(TestPass{"Cold Cache", nullptr, nullptr});
  testPasses.push_back(TestPass{"Warm Cache", refreshSampleTilesets, nullptr});

  return RunLoadTest(GetTestName(), setupForGoogleplex, testPasses);
}

bool FCesiumSampleLoadMontrealPointCloud::RunTest(const FString& Parameters) {

  auto adjustCamera = [this](SceneGenerationContext& context) {
    // Zoom way out
    context.startPosition = FVector(0, 0, 7240000.0);
    context.startRotation = FRotator(-90.0, 0.0, 0.0);
    context.syncWorldPlayerCamera();

    context.pawn->SetActorLocation(context.startPosition);
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
  testPasses.push_back(TestPass{"Cold Cache", nullptr, nullptr});
  testPasses.push_back(TestPass{"Adjust", adjustCamera, verifyVisibleTiles});

  return RunLoadTest(GetTestName(), setupForMontrealPointCloud, testPasses);
}

#endif
