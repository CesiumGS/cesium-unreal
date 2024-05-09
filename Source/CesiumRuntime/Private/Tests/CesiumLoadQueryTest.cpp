// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#if WITH_EDITOR

#include "CesiumLoadTestCore.h"

#include "Misc/AutomationTest.h"

#include "CesiumAsync/ICacheDatabase.h"
#include "CesiumGltfComponent.h"
#include "CesiumIonRasterOverlay.h"
#include "CesiumRuntime.h"
#include "CesiumSunSky.h"
#include "GlobeAwareDefaultPawn.h"

#include "Engine/StaticMeshActor.h"

using namespace Cesium;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCesiumTerrainQueryCityLocale,
    "Cesium.TerrainQuery.CityLocale",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

void setupCityLocale(SceneGenerationContext& context) {
  // Similar to SampleLocaleMelbourne
  context.setCommonProperties(
      FVector(144.951538, -37.809871, 140.334974),
      FVector(1052, 506, 23651),
      FRotator(-32, 20, 0),
      90.0f);

  context.sunSky->SolarTime = 16.8;
  context.sunSky->UpdateSun();

  ACesium3DTileset* melbourneBuildings =
      context.world->SpawnActor<ACesium3DTileset>();
  melbourneBuildings->SetTilesetSource(ETilesetSource::FromCesiumIon);
  melbourneBuildings->SetIonAssetID(69380);
  melbourneBuildings->SetIonAccessToken(SceneGenerationContext::testIonToken);
  melbourneBuildings->SetMaximumScreenSpaceError(6.0);
  melbourneBuildings->SetActorLabel(TEXT("Melbourne Photogrammetry"));
  melbourneBuildings->SetActorLocation(FVector(0, 0, 900));

  context.tilesets.push_back(melbourneBuildings);
}

bool FCesiumTerrainQueryCityLocale::RunTest(const FString& Parameters) {

  auto clearCache = [this](
                        SceneGenerationContext& context,
                        TestPass::TestingParameter parameter) {
    std::shared_ptr<CesiumAsync::ICacheDatabase> pCacheDatabase =
        getCacheDatabase();
    pCacheDatabase->clearAll();
  };

  struct TestResults {
    std::atomic<bool> queryFinished = false;
    std::vector<double> queryResults;
  };

  static TestResults testResults;

  auto issueQueries = [this, &testResults = testResults](
                          SceneGenerationContext& context,
                          TestPass::TestingParameter parameter) {
    // Test right at camera position
    CesiumGeospatial::Cartographic testInput =
        CesiumGeospatial::Cartographic::fromDegrees(144.951538, -37.80987);

    ACesium3DTileset* tileset = context.tilesets[0];
    Cesium3DTilesSelection::Tileset* nativeTileset = tileset->GetTileset();

    std::vector<CesiumGeospatial::Cartographic> queryInput = {testInput};

    nativeTileset->getHeightsAtCoordinates(queryInput)
        .thenInMainThread([&testResults](std::vector<double> results) {
          testResults.queryResults = results;
          testResults.queryFinished = true;
        });
  };

  auto waitForQueries = [this, &testResults = testResults](
                            SceneGenerationContext& creationContext,
                            SceneGenerationContext& playContext,
                            TestPass::TestingParameter parameter) {
    return (bool)testResults.queryFinished;
  };

  auto showResults = [this, &testResults = testResults](
                         SceneGenerationContext& creationContext,
                         SceneGenerationContext& playContext,
                         TestPass::TestingParameter parameter) {
    // Place some objects on the ground to verify positions
    UWorld* World = creationContext.world;

    FSoftObjectPath objectPath(
        TEXT("StaticMesh'/Engine/BasicShapes/Cube.Cube'"));
    TSoftObjectPtr<UStaticMesh> cubeMeshPtr =
        TSoftObjectPtr<UStaticMesh>(objectPath);

    // TODO, hook up to actual results
    AStaticMeshActor* staticMeshActor = World->SpawnActor<AStaticMeshActor>();
    staticMeshActor->GetStaticMeshComponent()->SetStaticMesh(cubeMeshPtr.Get());
    staticMeshActor->SetActorLocation(FVector(1, 1, 1));
    return true;
  };

  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Load city from cold cache", clearCache, nullptr});
  testPasses.push_back(
      TestPass{"Issue height queries and wait", issueQueries, waitForQueries});
  testPasses.push_back(
      TestPass{"Populate scene with results", nullptr, showResults});

  return RunLoadTest(
      GetBeautifiedTestName(),
      setupCityLocale,
      testPasses,
      1024,
      768);
}

#endif
