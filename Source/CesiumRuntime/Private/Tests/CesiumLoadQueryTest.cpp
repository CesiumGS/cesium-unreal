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
    Cesium3DTilesSelection::Tileset::HeightResults heightResults;
  };

  static TestResults testResults;

  auto issueQueries = [this, &testResults = testResults](
                          SceneGenerationContext& context,
                          TestPass::TestingParameter parameter) {
    // Test right at camera position
    double testLongitude = 144.951538;
    double testLatitude = -37.80987;

    // Make a grid of test points
    const size_t gridRowCount = 5;
    const size_t gridColumnCount = 5;
    double cartographicSpacing = 0.001;

    std::vector<CesiumGeospatial::Cartographic> queryInputRadians;

    for (size_t rowIndex = 0; rowIndex < gridRowCount; ++rowIndex) {
      double rowLatitude = testLatitude + (cartographicSpacing * rowIndex);

      for (size_t columnIndex = 0; columnIndex < gridColumnCount;
           ++columnIndex) {
        CesiumGeospatial::Cartographic queryInstance = {
            testLongitude + (cartographicSpacing * columnIndex),
            rowLatitude};

        queryInputRadians.push_back(CesiumGeospatial::Cartographic::fromDegrees(
            queryInstance.longitude,
            queryInstance.latitude));
      }
    }

    ACesium3DTileset* tileset = context.tilesets[0];
    Cesium3DTilesSelection::Tileset* nativeTileset = tileset->GetTileset();

    nativeTileset->getHeightsAtCoordinates(queryInputRadians)
        .thenInMainThread(
            [&testResults](
                Cesium3DTilesSelection::Tileset::HeightResults&& results) {
              testResults.heightResults = std::move(results);
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
    // Turn on the editor tileset updates so we can see what we loaded
    creationContext.setSuspendUpdate(false);

    // Place an object on the ground to verify position
    UWorld* World = creationContext.world;

    FSoftObjectPath objectPath(
        TEXT("StaticMesh'/Engine/BasicShapes/Cube.Cube'"));
    TSoftObjectPtr<UStaticMesh> cubeMeshPtr =
        TSoftObjectPtr<UStaticMesh>(objectPath);

    ACesium3DTileset* tileset = playContext.tilesets[0];
    Cesium3DTilesSelection::Tileset* nativeTileset = tileset->GetTileset();

    size_t resultCount = testResults.heightResults.coordinateResults.size();
    for (size_t resultIndex = 0; resultIndex < resultCount; ++resultIndex) {
      auto& coordinateResult =
          testResults.heightResults.coordinateResults[resultIndex];

      // Log any warnings
      for (std::string& warning : coordinateResult.warnings) {
        UE_LOG(
            LogCesium,
            Warning,
            TEXT("Height query traversal warning: %s"),
            warning.c_str());
      }

      if (!coordinateResult.heightAvailable)
        continue;

      CesiumGeospatial::Cartographic& queryHit = coordinateResult.coordinate;

      FVector hitCoordinate = {
          CesiumUtility::Math::radiansToDegrees(queryHit.longitude),
          CesiumUtility::Math::radiansToDegrees(queryHit.latitude),
          queryHit.height};

      FVector unrealPosition =
          tileset->ResolveGeoreference()
              ->TransformLongitudeLatitudeHeightPositionToUnreal(hitCoordinate);

      AStaticMeshActor* staticMeshActor = World->SpawnActor<AStaticMeshActor>();
      staticMeshActor->GetStaticMeshComponent()->SetStaticMesh(
          cubeMeshPtr.Get());
      staticMeshActor->SetActorLocation(unrealPosition);
      staticMeshActor->SetActorScale3D(FVector(10, 10, 10));
      staticMeshActor->SetActorLabel(
          FString::Printf(TEXT("Hit %d"), resultIndex));
      staticMeshActor->SetFolderPath("/QueryResults");
    }

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
