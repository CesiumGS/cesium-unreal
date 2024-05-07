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

using namespace Cesium;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCesiumTerrainQueryCityLocale,
    "Cesium.TerrainQuery.CityLocale",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::PerfFilter)

void setupCityLocale(SceneGenerationContext& context) {
  // Similar to Melbourne
  context.setCommonProperties(
      FVector(144.951538, -37.809871, 140.334974),
      FVector(1052, 506, 23651),
      FRotator(-32, 20, 0),
      90.0f);

  context.sunSky->SolarTime = 16.8;
  context.sunSky->UpdateSun();

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

  ACesium3DTileset* melbourneTileset =
      context.world->SpawnActor<ACesium3DTileset>();
  melbourneTileset->SetTilesetSource(ETilesetSource::FromCesiumIon);
  melbourneTileset->SetIonAssetID(69380);
  melbourneTileset->SetIonAccessToken(SceneGenerationContext::testIonToken);
  melbourneTileset->SetMaximumScreenSpaceError(6.0);
  melbourneTileset->SetActorLabel(TEXT("Melbourne Photogrammetry"));
  melbourneTileset->SetActorLocation(FVector(0, 0, 900));

  context.tilesets.push_back(worldTerrainTileset);
  context.tilesets.push_back(melbourneTileset);
}

bool FCesiumTerrainQueryCityLocale::RunTest(const FString& Parameters) {

  auto clearCache = [this](
                        SceneGenerationContext& context,
                        TestPass::TestingParameter parameter) {
    std::shared_ptr<CesiumAsync::ICacheDatabase> pCacheDatabase =
        getCacheDatabase();
    pCacheDatabase->clearAll();
  };

  auto issueQueries = [this](
                          SceneGenerationContext& context,
                          TestPass::TestingParameter parameter) {
    // TODO
  };

  auto waitForQueries = [this](
                            SceneGenerationContext& context,
                            TestPass::TestingParameter parameter) {
    // TODO
  };

  std::vector<TestPass> testPasses;
  testPasses.push_back(
      TestPass{"Load city from cold cache", clearCache, nullptr});
  testPasses.push_back(
      TestPass{"Issue height queries and wait", issueQueries, waitForQueries});

  return RunLoadTest(
      GetBeautifiedTestName(),
      setupCityLocale,
      testPasses,
      1024,
      768);
}

#endif
