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
using namespace std::chrono_literals;

namespace {
void setupDenverHillsCesiumWorldTerrain(SceneGenerationContext& context);
void setupDenverHillsGoogle(SceneGenerationContext& context);
bool RunSingleQueryTest(
    const FString& testName,
    std::function<void(SceneGenerationContext&)> setup);
bool RunMultipleQueryTest(
    const FString& testName,
    std::function<void(SceneGenerationContext&)> setup);
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSampleHeightMostDetailedCesiumWorldTerrainSingle,
    "Cesium.Performance.SampleHeightMostDetailed.Single query against Cesium World Terrain",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSampleHeightMostDetailedCesiumWorldTerrainMultiple,
    "Cesium.Performance.SampleHeightMostDetailed.Multiple queries against Cesium World Terrain",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSampleHeightMostDetailedGoogleSingle,
    "Cesium.Performance.SampleHeightMostDetailed.Single query against Google Photorealistic 3D Tiles",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSampleHeightMostDetailedGoogleMultiple,
    "Cesium.Performance.SampleHeightMostDetailed.Multiple queries against Google Photorealistic 3D Tiles",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

bool FSampleHeightMostDetailedCesiumWorldTerrainSingle::RunTest(
    const FString& Parameters) {
  return RunSingleQueryTest(
      this->GetBeautifiedTestName(),
      setupDenverHillsCesiumWorldTerrain);
}

bool FSampleHeightMostDetailedCesiumWorldTerrainMultiple::RunTest(
    const FString& Parameters) {
  return RunMultipleQueryTest(
      this->GetBeautifiedTestName(),
      setupDenverHillsCesiumWorldTerrain);
}

bool FSampleHeightMostDetailedGoogleSingle::RunTest(const FString& Parameters) {
  return RunSingleQueryTest(
      this->GetBeautifiedTestName(),
      setupDenverHillsGoogle);
}

bool FSampleHeightMostDetailedGoogleMultiple::RunTest(
    const FString& Parameters) {
  return RunMultipleQueryTest(
      this->GetBeautifiedTestName(),
      setupDenverHillsGoogle);
}

namespace {
// Our test model path
//
// Uses a simple cube, but to see trees instead, download 'temperate Vegetation:
// Spruce Forest' from the Unreal Engine Marketplace then use the following
// path...
// "'/Game/PN_interactiveSpruceForest/Meshes/full/low/spruce_full_01_low.spruce_full_01_low'"
FString terrainQueryTestModelPath(
    TEXT("StaticMesh'/Engine/BasicShapes/Cube.Cube'"));

void setupDenverHillsCesiumWorldTerrain(SceneGenerationContext& context) {
  context.setCommonProperties(
      FVector(-105.238887, 39.756177, 1887.175525),
      FVector(0, 0, 0),
      FRotator(-7, -226, -5),
      90.0f);

  // Add Cesium World Terrain
  ACesium3DTileset* worldTerrainTileset =
      context.world->SpawnActor<ACesium3DTileset>();
  worldTerrainTileset->SetTilesetSource(ETilesetSource::FromCesiumIon);
  worldTerrainTileset->SetIonAssetID(1);
  worldTerrainTileset->SetIonAccessToken(SceneGenerationContext::testIonToken);
  worldTerrainTileset->SetActorLabel(TEXT("Cesium World Terrain"));
  worldTerrainTileset->MaximumCachedBytes = 0;

  context.tilesets.push_back(worldTerrainTileset);
}

void setupDenverHillsGoogle(SceneGenerationContext& context) {
  context.setCommonProperties(
      FVector(-105.238887, 39.756177, 1887.175525),
      FVector(0, 0, 0),
      FRotator(-7, -226, -5),
      90.0f);

  // Add Cesium World Terrain
  ACesium3DTileset* googleTileset =
      context.world->SpawnActor<ACesium3DTileset>();
  googleTileset->SetTilesetSource(ETilesetSource::FromCesiumIon);
  googleTileset->SetIonAssetID(2275207);
  googleTileset->SetIonAccessToken(SceneGenerationContext::testIonToken);
  googleTileset->SetActorLabel(TEXT("Google Photorealistic 3D Tiles"));
  googleTileset->MaximumCachedBytes = 0;

  context.tilesets.push_back(googleTileset);
}

bool RunSingleQueryTest(
    const FString& testName,
    std::function<void(SceneGenerationContext&)> setup) {
  auto clearCache = [](SceneGenerationContext& context,
                       TestPass::TestingParameter parameter) {
    std::shared_ptr<CesiumAsync::ICacheDatabase> pCacheDatabase =
        getCacheDatabase();
    pCacheDatabase->clearAll();
  };

  struct TestResults {
    std::atomic<bool> queryFinished = false;
    TArray<FCesiumSampleHeightResult> heightResults;
    TArray<FString> warnings;
  };

  static TestResults testResults;

  auto issueQueries = [&testResults = testResults](
                          SceneGenerationContext& context,
                          TestPass::TestingParameter parameter) {
    // Test right at camera position
    double testLongitude = -105.257595;
    double testLatitude = 39.743103;

    // Make a grid of test points
    const size_t gridRowCount = 20;
    const size_t gridColumnCount = 20;
    double cartographicSpacing = 0.001;

    TArray<FVector> queryInput;

    for (size_t rowIndex = 0; rowIndex < gridRowCount; ++rowIndex) {
      double rowLatitude = testLatitude + (cartographicSpacing * rowIndex);

      for (size_t columnIndex = 0; columnIndex < gridColumnCount;
           ++columnIndex) {
        FVector queryInstance = {
            testLongitude + (cartographicSpacing * columnIndex),
            rowLatitude,
            0.0};

        queryInput.Add(queryInstance);
      }
    }

    ACesium3DTileset* tileset = context.tilesets[0];

    tileset->SampleHeightMostDetailed(
        queryInput,
        FCesiumSampleHeightMostDetailedCallback::CreateLambda(
            [&testResults](
                ACesium3DTileset* Tileset,
                const TArray<FCesiumSampleHeightResult>& Results,
                const TArray<FString>& Warnings) {
              testResults.heightResults = Results;
              testResults.warnings = Warnings;
              testResults.queryFinished = true;
            }));
  };

  auto waitForQueries = [&testResults = testResults](
                            SceneGenerationContext& creationContext,
                            SceneGenerationContext& playContext,
                            TestPass::TestingParameter parameter) {
    return (bool)testResults.queryFinished;
  };

  auto showResults = [&testResults = testResults](
                         SceneGenerationContext& creationContext,
                         SceneGenerationContext& playContext,
                         TestPass::TestingParameter parameter) {
    // Turn on the editor tileset updates so we can see what we loaded
    creationContext.setSuspendUpdate(false);

    // Place an object on the ground to verify position
    UWorld* World = creationContext.world;

    UStaticMesh* testMesh =
        LoadObject<UStaticMesh>(nullptr, *terrainQueryTestModelPath);

    ACesium3DTileset* tileset = playContext.tilesets[0];
    Cesium3DTilesSelection::Tileset* nativeTileset = tileset->GetTileset();

    // Log any warnings
    for (const FString& warning : testResults.warnings) {
      UE_LOG(LogCesium, Warning, TEXT("Height query warning: %s"), *warning);
    }

    int32 resultCount = testResults.heightResults.Num();
    for (int32 resultIndex = 0; resultIndex < resultCount; ++resultIndex) {
      const FVector& queryLongitudeLatitudeHeight =
          testResults.heightResults[resultIndex].LongitudeLatitudeHeight;

      if (!testResults.heightResults[resultIndex].SampleSuccess) {
        UE_LOG(
            LogCesium,
            Error,
            TEXT("The height at (%f,%f) was not sampled successfully."),
            queryLongitudeLatitudeHeight.X,
            queryLongitudeLatitudeHeight.Y);
        continue;
      }

      FVector unrealPosition =
          tileset->ResolveGeoreference()
              ->TransformLongitudeLatitudeHeightPositionToUnreal(
                  queryLongitudeLatitudeHeight);

      // Now bring the hit point to unreal world coordinates
      FVector unrealWorldPosition =
          tileset->GetActorTransform().TransformFVector4(unrealPosition);

      AStaticMeshActor* staticMeshActor = World->SpawnActor<AStaticMeshActor>();
      staticMeshActor->GetStaticMeshComponent()->SetStaticMesh(testMesh);
      staticMeshActor->SetActorLocation(unrealWorldPosition);
      staticMeshActor->SetActorScale3D(FVector(7, 7, 7));
      staticMeshActor->SetActorLabel(
          FString::Printf(TEXT("Hit %d"), resultIndex));
      staticMeshActor->SetFolderPath("/QueryResults");
    }

    return true;
  };

  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Load terrain from cold cache", clearCache, nullptr});
  testPasses.push_back(
      TestPass{"Issue height queries and wait", issueQueries, waitForQueries});
  testPasses.push_back(
      TestPass{"Populate scene with results", nullptr, showResults});

  return RunLoadTest(testName, setup, testPasses, 1280, 768);
}

bool RunMultipleQueryTest(
    const FString& testName,
    std::function<void(SceneGenerationContext&)> setup) {
  struct QueryObject {
    FVector coordinateDegrees;

    AStaticMeshActor* creationMeshActor = nullptr;
    AStaticMeshActor* playMeshActor = nullptr;

    bool queryFinished = false;
  };

  struct TestProcess {
    std::vector<QueryObject> queryObjects;
  };

  auto pProcess = std::make_shared<TestProcess>();

  //
  // Setup all object positions that will receive queries
  //
  // Test right at camera position
  double testLongitude = -105.257595;
  double testLatitude = 39.743103;

  // Make a grid of test points
  const size_t gridRowCount = 20;
  const size_t gridColumnCount = 20;
  double cartographicSpacing = 0.001;

  for (size_t rowIndex = 0; rowIndex < gridRowCount; ++rowIndex) {
    double rowLatitude = testLatitude + (cartographicSpacing * rowIndex);

    for (size_t columnIndex = 0; columnIndex < gridColumnCount; ++columnIndex) {
      FVector position(
          testLongitude + (cartographicSpacing * columnIndex),
          rowLatitude,
          2190.0);

      QueryObject newQueryObject = {position};

      pProcess->queryObjects.push_back(std::move(newQueryObject));
    }
  }

  auto clearCache = [](SceneGenerationContext&, TestPass::TestingParameter) {
    std::shared_ptr<CesiumAsync::ICacheDatabase> pCacheDatabase =
        getCacheDatabase();
    pCacheDatabase->clearAll();
  };

  auto addTestObjects = [pProcess](
                            SceneGenerationContext& creationContext,
                            SceneGenerationContext& playContext,
                            TestPass::TestingParameter) {
    // Place an object on the ground to verify position
    UWorld* creationWorld = creationContext.world;
    UWorld* playWorld = playContext.world;

    UStaticMesh* testMesh =
        LoadObject<UStaticMesh>(nullptr, *terrainQueryTestModelPath);

    ACesium3DTileset* tileset = playContext.tilesets[0];
    Cesium3DTilesSelection::Tileset* nativeTileset = tileset->GetTileset();

    for (size_t queryIndex = 0; queryIndex < pProcess->queryObjects.size();
         ++queryIndex) {
      QueryObject& queryObject = pProcess->queryObjects[queryIndex];

      FVector unrealPosition =
          tileset->ResolveGeoreference()
              ->TransformLongitudeLatitudeHeightPositionToUnreal(
                  queryObject.coordinateDegrees);

      // Now bring the hit point to unreal world coordinates
      FVector unrealWorldPosition =
          tileset->GetActorTransform().TransformFVector4(unrealPosition);

      {
        AStaticMeshActor* staticMeshActor =
            creationWorld->SpawnActor<AStaticMeshActor>();
        staticMeshActor->SetMobility(EComponentMobility::Movable);
        staticMeshActor->GetStaticMeshComponent()->SetStaticMesh(testMesh);
        staticMeshActor->SetActorLocation(unrealWorldPosition);
        staticMeshActor->SetActorScale3D(FVector(7, 7, 7));
        staticMeshActor->SetActorLabel(
            FString::Printf(TEXT("Hit %d"), queryIndex));
        staticMeshActor->SetFolderPath("/QueryResults");
        queryObject.creationMeshActor = staticMeshActor;
      }

      {
        AStaticMeshActor* staticMeshActor =
            playWorld->SpawnActor<AStaticMeshActor>();
        staticMeshActor->SetMobility(EComponentMobility::Movable);
        staticMeshActor->GetStaticMeshComponent()->SetStaticMesh(testMesh);
        staticMeshActor->SetActorLocation(unrealWorldPosition);
        staticMeshActor->SetActorScale3D(FVector(7, 7, 7));
        staticMeshActor->SetActorLabel(
            FString::Printf(TEXT("Hit %d"), queryIndex));
        staticMeshActor->SetFolderPath("/QueryResults");
        queryObject.playMeshActor = staticMeshActor;
      }
    }
    return true;
  };

  auto issueQueries = [pProcess](
                          SceneGenerationContext& context,
                          TestPass::TestingParameter) {
    ACesium3DTileset* tileset = context.tilesets[0];

    for (QueryObject& queryObject : pProcess->queryObjects) {
      tileset->SampleHeightMostDetailed(
          {queryObject.coordinateDegrees},
          FCesiumSampleHeightMostDetailedCallback::CreateLambda(
              [tileset, &queryObject](
                  ACesium3DTileset* pTileset,
                  const TArray<FCesiumSampleHeightResult>& results,
                  const TArray<FString>& warnings) {
                queryObject.queryFinished = true;

                // Log any warnings
                for (const FString& warning : warnings) {
                  UE_LOG(
                      LogCesium,
                      Warning,
                      TEXT("Height query traversal warning: %s"),
                      *warning);
                }

                if (results.Num() != 1) {
                  UE_LOG(
                      LogCesium,
                      Warning,
                      TEXT("Unexpected number of results received"));
                  return;
                }

                const FVector& newCoordinate =
                    results[0].LongitudeLatitudeHeight;
                if (!results[0].SampleSuccess) {
                  UE_LOG(
                      LogCesium,
                      Error,
                      TEXT(
                          "The height at (%f,%f) was not sampled successfully."),
                      newCoordinate.X,
                      newCoordinate.Y);
                  return;
                }

                const FVector& originalCoordinate =
                    queryObject.coordinateDegrees;

                if (!FMath::IsNearlyEqual(
                        originalCoordinate.X,
                        newCoordinate.X,
                        1e-12) ||
                    !FMath::IsNearlyEqual(
                        originalCoordinate.Y,
                        newCoordinate.Y,
                        1e-12)) {
                  UE_LOG(
                      LogCesium,
                      Warning,
                      TEXT("Hit result doesn't match original input"));
                  return;
                }

                FVector unrealPosition =
                    tileset->ResolveGeoreference()
                        ->TransformLongitudeLatitudeHeightPositionToUnreal(
                            newCoordinate);

                // Now bring the hit point to unreal world coordinates
                FVector unrealWorldPosition =
                    tileset->GetActorTransform().TransformFVector4(
                        unrealPosition);

                queryObject.creationMeshActor->SetActorLocation(
                    unrealWorldPosition);

                queryObject.playMeshActor->SetActorLocation(
                    unrealWorldPosition);
              }));
    }
  };

  auto waitForQueries = [pProcess](
                            SceneGenerationContext&,
                            SceneGenerationContext&,
                            TestPass::TestingParameter) {
    for (QueryObject& queryObject : pProcess->queryObjects) {
      if (!queryObject.queryFinished)
        return false;
    }
    return true;
  };

  auto showResults = [](SceneGenerationContext& creationContext,
                        SceneGenerationContext&,
                        TestPass::TestingParameter) {
    // Turn on the editor tileset updates so we can see what we loaded
    creationContext.setSuspendUpdate(false);
    return true;
  };

  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Load terrain from cold cache", clearCache, nullptr});
  testPasses.push_back(TestPass{"Add test objects", nullptr, addTestObjects});
  testPasses.push_back(
      TestPass{"Issue height queries and wait", issueQueries, waitForQueries});
  testPasses.push_back(TestPass{"Show results", nullptr, showResults});

  return RunLoadTest(testName, setup, testPasses, 1280, 720);
}

} // namespace

#endif
