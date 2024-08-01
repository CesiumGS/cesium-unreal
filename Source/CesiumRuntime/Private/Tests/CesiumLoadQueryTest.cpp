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
    FCesiumTerrainQuerySingleQuery,
    "Cesium.TerrainQuery.SingleQuery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCesiumTerrainQueryMultipleQueries,
    "Cesium.TerrainQuery.MultipleQueries",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

// Our test model path
//
// Uses a simple cube, but to see trees instead, download 'temperate Vegetation:
// Spruce Forest' from the Unreal Engine Marketplace then use the following
// path...
// "'/Game/PN_interactiveSpruceForest/Meshes/full/low/spruce_full_01_low.spruce_full_01_low'"
FString terrainQueryTestModelPath(
    TEXT("StaticMesh'/Engine/BasicShapes/Cube.Cube'"));

void setupDenverHills(SceneGenerationContext& context) {
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

  // Bing Maps Aerial overlay
  UCesiumIonRasterOverlay* pOverlay = NewObject<UCesiumIonRasterOverlay>(
      worldTerrainTileset,
      FName("Bing Maps Aerial"),
      RF_Transactional);
  pOverlay->MaterialLayerKey = TEXT("Overlay0");
  pOverlay->IonAssetID = 2;
  pOverlay->SetActive(true);
  pOverlay->OnComponentCreated();
  worldTerrainTileset->AddInstanceComponent(pOverlay);

  // Aerometrex Denver
  ACesium3DTileset* aerometrexTileset =
      context.world->SpawnActor<ACesium3DTileset>();
  aerometrexTileset->SetTilesetSource(ETilesetSource::FromCesiumIon);
  aerometrexTileset->SetIonAssetID(354307);
  aerometrexTileset->SetIonAccessToken(SceneGenerationContext::testIonToken);
  aerometrexTileset->SetMaximumScreenSpaceError(2.0);
  aerometrexTileset->SetActorLabel(TEXT("Aerometrex Denver"));

  context.tilesets.push_back(worldTerrainTileset);
  context.tilesets.push_back(aerometrexTileset);
}

bool FCesiumTerrainQuerySingleQuery::RunTest(const FString& Parameters) {

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
    double testLongitude = -105.257595;
    double testLatitude = 39.743103;

    // Make a grid of test points
    const size_t gridRowCount = 20;
    const size_t gridColumnCount = 20;
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

    UStaticMesh* testMesh =
        LoadObject<UStaticMesh>(nullptr, *terrainQueryTestModelPath);

    ACesium3DTileset* tileset = playContext.tilesets[0];
    Cesium3DTilesSelection::Tileset* nativeTileset = tileset->GetTileset();

    // Log any warnings
    for (auto& warning : testResults.heightResults.warnings) {
      UE_LOG(
          LogCesium,
          Warning,
          TEXT("Height query traversal warning: %s"),
          UTF8_TO_TCHAR(warning.c_str()));
    }

    size_t resultCount = testResults.heightResults.coordinateResults.size();
    for (size_t resultIndex = 0; resultIndex < resultCount; ++resultIndex) {
      auto& coordinateResult =
          testResults.heightResults.coordinateResults[resultIndex];

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

  return RunLoadTest(
      GetBeautifiedTestName(),
      setupDenverHills,
      testPasses,
      1280,
      768);
}

bool FCesiumTerrainQueryMultipleQueries::RunTest(const FString& Parameters) {

  struct QueryObject {
    CesiumGeospatial::Cartographic coordinateDegrees;
    CesiumGeospatial::Cartographic coordinateRadians;

    AStaticMeshActor* creationMeshActor = nullptr;
    AStaticMeshActor* playMeshActor = nullptr;

    bool queryFinished = false;
  };

  static std::vector<QueryObject> queryObjects;

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
      CesiumGeospatial::Cartographic cartographicInstance = {
          testLongitude + (cartographicSpacing * columnIndex),
          rowLatitude};

      QueryObject newQueryObject = {
          cartographicInstance,
          CesiumGeospatial::Cartographic::fromDegrees(
              cartographicInstance.longitude,
              cartographicInstance.latitude)};

      queryObjects.push_back(std::move(newQueryObject));
    }
  }

  auto clearCache = [](SceneGenerationContext&, TestPass::TestingParameter) {
    std::shared_ptr<CesiumAsync::ICacheDatabase> pCacheDatabase =
        getCacheDatabase();
    pCacheDatabase->clearAll();
  };

  auto addTestObjects = [&queryObjects = queryObjects](
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

    for (size_t queryIndex = 0; queryIndex < queryObjects.size();
         ++queryIndex) {
      QueryObject& queryObject = queryObjects[queryIndex];

      FVector startCoordinate = {
          queryObject.coordinateDegrees.longitude,
          queryObject.coordinateDegrees.latitude,
          2190};

      FVector unrealPosition =
          tileset->ResolveGeoreference()
              ->TransformLongitudeLatitudeHeightPositionToUnreal(
                  startCoordinate);

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

  auto issueQueries = [this, &queryObjects = queryObjects](
                          SceneGenerationContext& context,
                          TestPass::TestingParameter) {
    ACesium3DTileset* tileset = context.tilesets[0];
    Cesium3DTilesSelection::Tileset* nativeTileset = tileset->GetTileset();

    for (QueryObject& queryObject : queryObjects) {

      std::vector<CesiumGeospatial::Cartographic> queryInputRadians;
      queryInputRadians.push_back(queryObject.coordinateRadians);

      nativeTileset->getHeightsAtCoordinates(queryInputRadians)
          .thenInMainThread(
              [&queryObject, tileset](
                  Cesium3DTilesSelection::Tileset::HeightResults&& results) {
                queryObject.queryFinished = true;

                // Log any warnings
                for (auto& warning : results.warnings) {
                  UE_LOG(
                      LogCesium,
                      Warning,
                      TEXT("Height query traversal warning: %s"),
                      UTF8_TO_TCHAR(warning.c_str()));
                }

                if (results.coordinateResults.size() != 1) {
                  UE_LOG(
                      LogCesium,
                      Warning,
                      TEXT("Unexpected number of results received"));
                  return;
                }

                auto& coordinateResult = results.coordinateResults[0];

                auto& originalCoordinate = queryObject.coordinateRadians;
                auto& newCoordinate = coordinateResult.coordinate;

                if (originalCoordinate.latitude != newCoordinate.latitude ||
                    originalCoordinate.longitude != newCoordinate.longitude) {
                  UE_LOG(
                      LogCesium,
                      Warning,
                      TEXT("Hit result doesn't match original input"));
                  return;
                }

                FVector hitCoordinate = {
                    queryObject.coordinateDegrees.longitude,
                    queryObject.coordinateDegrees.latitude,
                    newCoordinate.height};

                FVector unrealPosition =
                    tileset->ResolveGeoreference()
                        ->TransformLongitudeLatitudeHeightPositionToUnreal(
                            hitCoordinate);

                // Now bring the hit point to unreal world coordinates
                FVector unrealWorldPosition =
                    tileset->GetActorTransform().TransformFVector4(
                        unrealPosition);

                queryObject.creationMeshActor->SetActorLocation(
                    unrealWorldPosition);

                queryObject.playMeshActor->SetActorLocation(
                    unrealWorldPosition);
              });
    }
  };

  auto waitForQueries = [this, &queryObjects = queryObjects](
                            SceneGenerationContext&,
                            SceneGenerationContext&,
                            TestPass::TestingParameter) {
    for (QueryObject& queryObject : queryObjects) {
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

  return RunLoadTest(
      GetBeautifiedTestName(),
      setupDenverHills,
      testPasses,
      1280,
      720);
}

#endif
