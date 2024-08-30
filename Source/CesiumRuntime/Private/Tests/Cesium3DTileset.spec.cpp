// Copyright 2020-2024 CesiumGS, Inc. and Contributors

#include "Cesium3DTileset.h"
#include "CesiumGlobeAnchorComponent.h"
#include "CesiumLoadTestCore.h"
#include "CesiumSceneGeneration.h"
#include "CesiumSunSky.h"
#include "CesiumTestHelpers.h"
#include "Engine/World.h"
#include "GlobeAwareDefaultPawn.h"
#include "Misc/AutomationTest.h"
#include <CesiumAsync/ICacheDatabase.h>

#define TEST_SCREEN_WIDTH 1280
#define TEST_SCREEN_HEIGHT 720

namespace Cesium {

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCesium3DTilesetSharedImages,
    "Cesium.Unit.3DTileset.SharedImages",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

static void setupForSharedImages(SceneGenerationContext& context) {
  context.setCommonProperties(
      FVector(21.16677692, -67.38013505, -6375355.1944),
      FVector(-12, -1300, -5),
      FRotator(0, 90, 0),
      60.0f);

  context.georeference->SetOriginEarthCenteredEarthFixed(FVector(0, 0, 0));
  context.pawn->SetActorLocation(FVector(-12, -1300, -5));

  context.sunSky->TimeZone = 9.0f;
  context.sunSky->UpdateSun();

  ACesiumGeoreference* georeference =
      context.world->SpawnActor<ACesiumGeoreference>();
  check(georeference != nullptr);
  georeference->SetOriginPlacement(EOriginPlacement::TrueOrigin);

  ACesium3DTileset* tileset = context.world->SpawnActor<ACesium3DTileset>();
  tileset->SetTilesetSource(ETilesetSource::FromUrl);
  // Unreal returns the relative path of the plugins directory by default
  FString FullPluginsPath =
      IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(
          *FPaths::ProjectPluginsDir());
  tileset->SetUrl(FString::Printf(
      TEXT(
          "file://%scesium-unreal/extern/cesium-native/Cesium3DTilesSelection/test/data/SharedImages/tileset.json"),
      *FullPluginsPath));

  tileset->SetActorLabel(TEXT("SharedImages"));
  tileset->SetGeoreference(georeference);
  tileset->SuspendUpdate = false;
  context.tilesets.push_back(tileset);

  UCesiumGlobeAnchorComponent* GlobeAnchorComponent =
      NewObject<UCesiumGlobeAnchorComponent>(tileset, TEXT("GlobeAnchor"));
  tileset->AddInstanceComponent(GlobeAnchorComponent);
  GlobeAnchorComponent->SetAdjustOrientationForGlobeWhenMoving(false);
  GlobeAnchorComponent->SetGeoreference(georeference);
  GlobeAnchorComponent->RegisterComponent();
  GlobeAnchorComponent->MoveToEarthCenteredEarthFixedPosition(
      FVector(0.0, 0.0, 0.0));
}

void tilesetPass(
    SceneGenerationContext& context,
    TestPass::TestingParameter parameter) {
  context.refreshTilesets();
  context.setSuspendUpdate(false);
}

bool FCesium3DTilesetSharedImages::RunTest(const FString& Parameters) {
  std::vector<TestPass> testPasses;
  testPasses.push_back(TestPass{"Refresh Pass", tilesetPass, nullptr});

  return RunLoadTest(
      GetBeautifiedTestName(),
      setupForSharedImages,
      testPasses,
      TEST_SCREEN_WIDTH,
      TEST_SCREEN_HEIGHT);
}

} // namespace Cesium
